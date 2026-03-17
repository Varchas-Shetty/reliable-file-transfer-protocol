#include <stdio.h>
#include <string.h>
#include "protocol.h"

// Fill a message struct
void create_message(Message *msg, int type, const char *data) {
    msg->type = type;
    msg->seq_no = 0;  // default
    if (data != NULL) {
        strncpy(msg->data, data, CHUNK_SIZE);
        msg->data[CHUNK_SIZE - 1] = '\0'; // safety null terminator
        msg->data_size = strlen(msg->data);
    } else {
        msg->data[0] = '\0';
        msg->data_size = 0;
    }
}

// Print message for debugging
void print_message(Message *msg) {
    printf("Message(type=%d, seq=%d, size=%d, data=%s)\n",
           msg->type, msg->seq_no, msg->data_size, msg->data);
}