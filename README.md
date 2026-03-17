# Reliable File Transfer Protocol over UDP

## Overview
This project implements a **Reliable File Transfer Protocol over UDP** using socket programming. Since UDP does not guarantee reliability, this system introduces mechanisms such as chunk-based transmission, acknowledgements, retransmission of lost packets, and optional SSL/TLS-secured control communication.

The system ensures that files are transferred correctly even when packets are lost or delayed in the network.

---

## Problem Statement
UDP provides fast communication but does not guarantee delivery, ordering, or duplication control. This project builds a **custom reliability layer on top of UDP** to ensure safe and complete file transfer between a client and server.

---

## Features
- Chunk-based file transfer
- Acknowledgement (ACK) based reliability
- Retransmission of lost packets
- Packet loss simulation for testing
- SSL/TLS secured control communication
- Performance testing support

---

## System Architecture

```
Server                          Client
  |                                |
  |-------- UDP Data Packets ----->|
  |                                |
  |<----------- ACK ---------------|
  |                                |
  |---- Retransmit if ACK lost --->|
  |                                |
```

Flow:

1. Server splits the file into chunks.
2. Each chunk is sent as a packet over UDP.
3. Client sends an acknowledgement (ACK).
4. If ACK is not received, the server retransmits the packet.

---

## Project Structure

```
project-root/
│
├── client/              # Client-side (connects to server, sends requests)
│   ├── client.c         # main client logic (connect, send, receive)
│   ├── client_ssl.c     # SSL/TLS wrapper for client
│   ├── client_utils.c   # helper functions
│   └── include/
│       └── client.h     # declarations
│
├── server/              # Server-side (          sockets + concurrency)
│   ├── server.c         # main server (socket, bind, listen, accept)
│   ├── server_ssl.c     # SSL integration on server
│   ├── connection_handler.c  # handles each client
│   ├── thread_pool.c    # multi-client handling
│   └── include/
│       └── server.h     # declarations
│
├── common/              # Shared between client & server
│   ├── protocol.c       # message handling logic
│   ├── protocol.h       # message format definitions
│   ├── constants.h      # ports, buffer sizes
│   └── utils.c          # shared helpers
│
├── security/            # SSL/TLS setup
│   ├── ssl_setup.c      # SSL initialization
│   ├── ssl_setup.h
│   └── certs/           # certificates
│       ├── server.crt
│       ├── server.key
│       └── ca.crt
│
├── tests/               # Performance + load testing
│   ├── multi_client_test.c
│   ├── latency_test.c
│   └── load_test.c
│
├── docs/                # Documentation
│   ├── protocol.md
│   ├── architecture.md
│   └── performance.md
│
├── Makefile             # compile everything
└── README.md            # project overview
```

---

## Technologies Used
- C
- Python
- Socket Programming
- UDP Protocol
- SSL/TLS (Python ssl library)
- GitHub for version control

---

## How to Run the Project

### 1. Clone the repository

```
git clone https://github.com/YOUR_USERNAME/reliable-file-transfer-protocol.git
```

```
cd reliable-file-transfer-protocol
```

---

### 2. Start the Server

```
python server.py
```

Server will start listening for incoming packets.

---

### 3. Run the Client

```
python client.py
```

The client will begin sending the file in chunks.

---

## Example Output

### Server

```
Server listening on port 5000
Received file request from client
Sending chunk 0
ACK received for chunk 0
Sending chunk 1
ACK intentionally dropped for testing
Resending chunk 1
ACK received for chunk 1
File transfer complete
```

### Client

```
Requesting file: testfile.txt
Received chunk 0
ACK sent for chunk 0
Received chunk 1
Resending ACK for chunk 1
Received chunk 1
File saved as received_file
```

---

## Packet Structure

Each UDP packet contains:

```
| Type | Sequence Number | Data Size | Data |
```
- **Type** → identifies the kind of message (request, data, acknowledgment)
- **Sequence Number** → sequence number used to maintain packet order and detect missing packets
- **Data Size** → number of valid bytes in data
- **Data** → holds either a file chunk or a filename

---

## Reliability Mechanisms

The protocol ensures reliability using:

1. **Sequence numbers** to track packets
2. **Acknowledgements (ACK)** from client
3. **Timeout detection**
4. **Packet retransmission**

---

## Security Implementation

SSL/TLS is used to secure the **control communication channel** between the client and server.

This provides:

- Encryption
- Authentication
- Protection from man-in-the-middle attacks

---

## Performance Evaluation

Example results:

| File Size | Transfer Time | Throughput |
|----------|---------------|-----------|
| 1 MB | 1.2 sec | 0.83 MB/s |
| 5 MB | 5.6 sec | 0.89 MB/s |

Packet loss testing confirms that retransmission successfully restores missing packets.

---

## Team Members

- **Varchas Shetty** – SSL Implementation & Security
- **Vaishnavi Bandaru** – UDP File Transfer Server with concurrency, reliable stop-and-wait protocol, and packet retransmission.
- **Ujwal** – 

---

