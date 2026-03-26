# Architecture

## Final Design

The final implementation uses a **UDP client-server architecture** with:

- a custom reliability layer
- resume support
- CRC32 integrity verification
- DTLS-secured transfer sessions

The system is split into two stages:

1. A lightweight UDP control stage on the main server port where clients request a file and announce their resume position.
2. A dedicated per-transfer UDP data socket where the client and server establish a DTLS session and perform the secure file transfer.

## Server Components

- `server/server.c`: UDP control socket, session setup, metadata calculation
- `server/connection_handler.c`: DTLS session worker, sliding window, retransmissions
- `server/thread_pool.c`: detached thread helper

## Client Components

- `client/client.c`: CLI and interactive entry point
- `client/client_utils.c`: request flow, DTLS connection, resume logic, file writing, CRC verification

## Shared Components

- `common/protocol.c`: packet serialization and parsing
- `common/utils.c`: CRC32, file helpers, and input utilities
- `security/ssl_setup.c`: DTLS context and secure packet helpers

## Reliability Strategy

- Fixed-size chunking
- Cumulative ACKs
- Sliding window transmission
- Timeout-based retransmission
- Resume from the last fully written chunk

## Integrity Strategy

- CRC32 per chunk to detect corrupt datagrams
- CRC32 for the entire file after completion

## Security Strategy

- DTLS handshake on the dedicated transfer socket
- Encrypted/authenticated transfer of `START`, `DATA`, `ACK`, and `END`
