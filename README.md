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
- File reconstruction on the server
- SSL/TLS secured control communication
- Performance testing support

---

## System Architecture

```
Client                          Server
  |                                |
  |-------- UDP Data Packets ----->|
  |                                |
  |<----------- ACK ---------------|
  |                                |
  |---- Retransmit if ACK lost --->|
  |                                |
```

Flow:

1. Client splits the file into chunks.
2. Each chunk is sent as a packet over UDP.
3. Server sends an acknowledgement (ACK).
4. If ACK is not received, the client retransmits the packet.
5. Server reconstructs the file from received chunks.

---

## Project Structure

```
project-root/
│
├── client.py          # Client-side program
├── server.py          # Server-side program
├── protocol.py        # Packet format and parsing
├── utils.py           # Helper functions (chunking, file handling)
│
├── ssl/               # SSL/TLS implementation
│   ├── client_ssl.py
│   └── server_ssl.py
│
├── test_files/        # Files used for testing transfer
│
├── docs/              # Documentation
│   ├── architecture.md
│   └── performance.md
│
└── README.md
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
Received chunk 0
ACK sent for chunk 0
Received chunk 1
ACK dropped intentionally for chunk 1
Received chunk 1
ACK sent for chunk 1
File saved as received_file
```

### Client

```
Sending file: testfile.txt
ACK received for 0
Resending chunk 1
ACK received for 1
File transfer complete
```

---

## Packet Structure

Each UDP packet contains:

```
| Sequence Number | Data Length | Data |
```

- **Sequence Number** → identifies packet order
- **Data Length** → size of chunk
- **Data** → file bytes

---

## Reliability Mechanisms

The protocol ensures reliability using:

1. **Sequence numbers** to track packets
2. **Acknowledgements (ACK)** from server
3. **Timeout detection**
4. **Packet retransmission**
5. **File reconstruction on server**

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
- **Vaishnavi** – 
- **Ujwal** – 

---

