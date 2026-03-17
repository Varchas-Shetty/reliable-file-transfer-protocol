#ifndef PROTOCOL_H
#define PROTOCOL_H

#define CHUNK_SIZE 1024

// Message types
#define MSG_TYPE_FILE_REQUEST  6
#define MSG_TYPE_FILE_DATA     7
#define MSG_TYPE_ACK           8
#define MSG_TYPE_END           9

typedef struct {
    int type;               // message type
    int seq_no;             // sequence number for reliability
    int data_size;          // number of bytes used in data
    char data[CHUNK_SIZE];  // file chunk or filename
} Message;

#endif