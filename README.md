# reliable-file-transfer-protocol
Problem Statement :Reliable File Transfer Protocol over UDP.    This project implements a reliable file transfer system over UDP using socket programming.  The system ensures reliable delivery through chunk-based transmission, acknowledgements,  retransmission of lost packets, and optional SSL-secured control communication.
Features :
• Chunk-based file transfer
• ACK-based reliability
• Packet loss simulation
• Retransmission mechanism
• File reconstruction
• SSL/TLS secure control channel

PROJECT STRUCTURE
project-root/
│
├── client/              # Client-side (connects to server, sends requests)
│   ├── client.c         # main client logic (connect, send, receive)
│   ├── client_ssl.c     # SSL/TLS wrapper for client
│   ├── client_utils.c   # helper functions
│   └── include/
│       └── client.h     # declarations
│
├── server/              # Server-side (YOU → sockets + concurrency)
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
