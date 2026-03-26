#!/usr/bin/env python3
import argparse
import concurrent.futures
import pathlib
import statistics
import subprocess
import time


def run_client(client_bin: str, host: str, filename: str,
               output_dir: pathlib.Path, index: int, timeout: float):
    output_path = output_dir / f"download_{index}_{pathlib.Path(filename).name}"
    command = [client_bin, host, filename, str(output_path)]
    start = time.perf_counter()

    try:
        completed = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        duration = time.perf_counter() - start
        return {
            "index": index,
            "duration": duration,
            "returncode": completed.returncode,
            "stdout": completed.stdout,
            "stderr": completed.stderr,
            "output_path": output_path,
        }
    except subprocess.TimeoutExpired as exc:
        duration = time.perf_counter() - start
        return {
            "index": index,
            "duration": duration,
            "returncode": -1,
            "stdout": exc.stdout or "",
            "stderr": f"Timed out after {timeout:.1f}s",
            "output_path": output_path,
        }


def main():
    parser = argparse.ArgumentParser(
        description="Run concurrent client downloads for throughput measurements."
    )
    parser.add_argument("--client-bin", default="./client/client_app",
                        help="Path to the compiled client binary")
    parser.add_argument("--host", default="127.0.0.1",
                        help="Server hostname or IP")
    parser.add_argument("--filename", required=True,
                        help="Filename to request from the server")
    parser.add_argument("--clients", type=int, default=3,
                        help="Number of concurrent clients")
    parser.add_argument("--output-dir", default="tests/downloads",
                        help="Where to store temporary outputs")
    parser.add_argument("--timeout", type=float, default=20.0,
                        help="Timeout in seconds for each client run")
    args = parser.parse_args()

    output_dir = pathlib.Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    results = []
    overall_start = time.perf_counter()
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.clients) as executor:
        futures = [
            executor.submit(
                run_client,
                args.client_bin,
                args.host,
                args.filename,
                output_dir,
                i,
                args.timeout,
            )
            for i in range(args.clients)
        ]
        for future in concurrent.futures.as_completed(futures):
            results.append(future.result())
    overall_duration = time.perf_counter() - overall_start

    failures = [result for result in results if result["returncode"] != 0]
    successful = [result for result in results if result["returncode"] == 0]

    print(f"Clients launched: {args.clients}")
    print(f"Total wall-clock time: {overall_duration:.2f}s")

    if successful:
        durations = [result["duration"] for result in successful]
        print(f"Successful runs: {len(successful)}")
        print(f"Average client time: {statistics.mean(durations):.2f}s")
        print(f"Fastest client time: {min(durations):.2f}s")
        print(f"Slowest client time: {max(durations):.2f}s")

    if failures:
        print(f"Failures: {len(failures)}")
        for result in failures:
            print(f"\n[client {result['index']}] return code {result['returncode']}")
            if result["stderr"]:
                print(result["stderr"].strip())

    if not failures and successful:
        print("\nSample client output:")
        print(successful[0]["stdout"].strip())


if __name__ == "__main__":
    main()
