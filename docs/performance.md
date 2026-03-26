# Performance Evaluation Guide

## What To Measure

For the project presentation, measure:

- transfer time
- throughput
- behavior with multiple concurrent clients
- behavior when resuming an interrupted transfer

## Suggested Demo Cases

1. Small file transfer: `server/test.txt`
2. Larger file transfer: place a multi-megabyte file in `server/` so resume and the sliding window are visible
3. Concurrent downloads with 3 to 5 clients
4. Manual interruption followed by resume

## Example Commands

Start server:

```bash
make
./server/server_app
```

Single client:

```bash
./client/client_app test.txt
```

Concurrent run:

```bash
python3 tests/performance_test.py --filename test.txt --clients 5
```

Cross-system run:

```bash
python3 tests/performance_test.py --host 192.168.1.10 --filename test.txt --clients 5
```

## Metrics Table Template

| File | Clients | Resume Used | Transfer Time (s) | Throughput (KB/s) | Notes |
|------|---------|-------------|-------------------|-------------------|-------|
| test.txt | 1 | No | | | |
| sample.bin | 1 | No | | | |
| sample.bin | 1 | Yes | | | |
| sample.bin | 5 | No | | | |
