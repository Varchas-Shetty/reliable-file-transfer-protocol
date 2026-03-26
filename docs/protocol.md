# Protocol

## Message Types

- `REQUEST`: client asks for a file and includes its resume chunk
- `SESSION_INFO`: server returns metadata and the dedicated transfer port
- `START`: client confirms the transfer session and resume point
- `DATA`: one UDP file chunk
- `ACK`: cumulative acknowledgement using `next_expected_seq`
- `END`: transfer complete
- `ERROR`: protocol or file error

## Packet Fields

Each packet carries:

- `type`
- `session_id`
- `seq_no`
- `ack_no`
- `total_chunks`
- `data_size`
- `window_size`
- `checksum`
- `file_size`
- `port`
- `filename`
- `data`

## Security Model

- `REQUEST` and `SESSION_INFO` are exchanged on the control socket
- The dedicated transfer socket then performs a DTLS handshake
- `START`, `DATA`, `ACK`, and `END` are exchanged securely over DTLS

## Reliability Model

The data path uses **Go-Back-N style sliding-window transmission**:

- Server sends multiple chunks up to `WINDOW_SIZE`
- Client ACKs the next expected chunk number
- Server retransmits the unacknowledged window on timeout

This improves throughput compared with stop-and-wait.

## Resume Model

- Client inspects the existing output file
- Resume starts from the last chunk-aligned offset
- Client sends that chunk number in `REQUEST`
- Server confirms the accepted resume point in `SESSION_INFO`
- Client resumes securely after the DTLS handshake

## Integrity Model

- Each `DATA` packet includes a CRC32 of its payload
- The final `END` metadata includes the full-file CRC32
- Client recomputes the whole file CRC32 before declaring success
