# Reliable File Transfer Protocol over UDP with Go-Back-N ARQ and DTLS Security

## Overview
This project implements a Reliable File Transfer Protocol over UDP using socket programming, Go-Back-N ARQ reliability mechanisms, and DTLS-based security. Since UDP does not guarantee delivery, ordering, or duplication protection, this project adds a custom reliability layer with sequence numbers, cumulative acknowledgements, timeout-based retransmissions, sliding window transmission, and secure control communication.

The system ensures that files are transferred correctly even in the presence of packet loss or network delay.

---

## Problem Statement
UDP is fast but unreliable because:
- Packets may be lost
- Packets may arrive out of order
- Duplicate packets may occur
- No built-in retransmission

This project solves these problems by implementing a Reliable Data Transfer (RDT) protocol on top of UDP.

---

## Features
- Chunk-based file transfer
- Stop-and-Wait reliability protocol
- Packet acknowledgement (ACK)
- Packet retransmission on timeout
- Concurrent server handling multiple clients
- DTLS is used to secure control communication over UDP.
- Packet loss simulation for testing
- Performance testing support

---
```
### System Architecture

Server                          Client
  |                                |
  |-------- UDP Data Packets ----->|
  |                                |
  |<----------- ACK ---------------|
  |                                |
  |---- Retransmit if ACK lost --->|
  |                                |
```
### Working Flow
1. Client requests a file from server.
2. Server splits the file into chunks.
3. Each chunk is sent as a UDP packet.
4. Client sends ACK for each received packet.
5. If ACK not received → server retransmits packet.
6. DTLS is used to secure control communication over UDP.

---

## Project Structure

```
project-root/
│
├── client/
│   ├── client.c
│   ├── client_ssl.c
│   ├── client_utils.c
│   └── include/
│       └── client.h
│
├── server/
│   ├── server.c
│   ├── server_ssl.c
│   ├── connection_handler.c
│   ├── thread_pool.c
│   └── include/
│       └── server.h
│
├── common/
│   ├── protocol.c
│   ├── protocol.h
│   ├── constants.h
│   └── utils.c
│
├── security/
│   ├── ssl_setup.c
│   ├── ssl_setup.h
│   └── certs/
│       ├── server.crt
│       ├── server.key
│       └── ca.crt
│
├── tests/
├── docs/
├── Makefile
└── README.md
```

---

## Technologies Used
- C Programming
- Socket Programming
- UDP Protocol
- OpenSSL (SSL/TLS)
- Multi-threading
- VirtualBox (for multi-system testing)
- GitHub

---

## Packet Structure

| Field | Description |
|------|-------------|
| Type | Request / Data / ACK |
| Sequence Number | Packet order tracking |
| Data Size | Size of actual data |
| Data | File chunk |

---

## Reliability Mechanisms
Reliability is achieved using:

Sequence numbers
Cumulative acknowledgements (ACKs)
Sliding window transmission
Timeout detection
Packet retransmission (Go-Back-N recovery)
Go-Back-N ARQ protocol

---

## Security Implementation
DTLS (Datagram Transport Layer Security) is used to secure the control communication between client and server over UDP.

This provides:

Encryption
Authentication
Integrity protection
Protection against Man-in-the-Middle attacks

---

## Performance Evaluation

| File Size | Transfer Time | Throughput |
|-----------|---------------|------------|
| 1 MB | 1.2 sec | 0.83 MB/s |
| 5 MB | 5.6 sec | 0.89 MB/s |

---

# How to Run the Project

## Step 1 – Clone Repository
```bash
git clone https://github.com/YOUR_USERNAME/reliable-file-transfer-protocol.git
cd reliable-file-transfer-protocol
```

---

## Step 2 – Compile the Project

If using Makefile:
```bash
make
```

Or compile manually:
```bash
gcc server/server.c security/ssl_setup.c -o server -lssl -lcrypto -lpthread
gcc client/client.c security/ssl_setup.c -o client -lssl -lcrypto
```

---

# Running the Project

## Run on Single System (Same Laptop)

Open two terminals.

### Terminal 1 – Start Server
```bash
./server
```

### Terminal 2 – Start Client
```bash
./client 127.0.0.1
```

127.0.0.1 means localhost (same system).

---

## Run on Two Systems (Laptop + Virtual Machine)

### Step 1 – Configure VirtualBox Network
- Open VirtualBox
- Go to Settings → Network
- Select Bridged Adapter
- Start Virtual Machine

### Step 2 – Find Server IP Address
On server system:
```bash
ipconfig     (Windows)
ifconfig     (Linux)
```

Example IP:
```
192.168.1.5
```

### Step 3 – Start Server (on Host Machine)
```bash
./server
```

### Step 4 – Start Client (on VM)
```bash
./client 192.168.1.5
```

Now file transfer will happen between two different systems.

---

## Running Multiple Clients
To run multiple clients:
- Open multiple terminals (or multiple VMs)
- Run:
```bash
./client 192.168.1.5
```
Server thread pool will handle multiple clients concurrently.

---

## Expected Output

### Server Output
```
Server started...
Client connected...
Sending packet 0
ACK received
Sending packet 1
Timeout... Retransmitting packet 1
File transfer completed
```

### Client Output
```
Connected to server
Receiving file...
Packet received
ACK sent
File received successfully
```

---

## Team Members

| Name | Contribution |
|------|-------------|
| Varchas Shetty | Implemented SSL/TLS setup, certificate integration, and secure control-channel communication between client and server |
| Vaishnavi Bandaru | Developed the UDP-based file transfer server, including concurrency handling, Go-Back-N ARQ reliability logic, timeout management, sliding window transmission, and packet retransmission |
| Ujwal | Built the client-side file transfer logic, including request handling, in-order packet processing, cumulative ACK generation, and file reconstruction |

---

## Applications
- Secure file transfer systems
- Banking and financial data transfer
- Military communication
- Distributed systems
- Networking protocol research

---

## Future Improvements
- Resume interrupted transfer
- GUI Interface
- Performance graph visualization
- Cloud deployment

---

## Conclusion
This project demonstrates how reliability and security can be implemented over UDP using Go-Back-N ARQ, packet retransmission, SSL/TLS encryption, and a multi-client concurrent server architecture, enabling accurate, efficient, and secure file transfer even in unreliable network conditions.