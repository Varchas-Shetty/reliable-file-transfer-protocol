#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "constants.h"

typedef enum {
    MSG_TYPE_REQUEST = 1,
    MSG_TYPE_SESSION_INFO = 2,
    MSG_TYPE_START = 3,
    MSG_TYPE_DATA = 4,
    MSG_TYPE_ACK = 5,
    MSG_TYPE_END = 6,
    MSG_TYPE_ERROR = 7
} MessageType;

#pragma pack(push, 1)
typedef struct {
    uint32_t type;
    uint32_t session_id;
    uint32_t seq_no;
    uint32_t ack_no;
    uint32_t total_chunks;
    uint32_t data_size;
    uint32_t window_size;
    uint32_t checksum;
    uint32_t file_size;
    uint32_t port;
    char filename[MAX_FILENAME_LENGTH + 1];
    unsigned char data[MAX_PAYLOAD_SIZE];
} Packet;
#pragma pack(pop)

void init_packet(Packet *packet, uint32_t type);
void packet_host_to_network(Packet *packet);
int packet_network_to_host(Packet *packet);
ssize_t send_packet(int sockfd, const Packet *packet,
                    const struct sockaddr_in *addr, socklen_t addrlen);
ssize_t recv_packet(int sockfd, Packet *packet,
                    struct sockaddr_in *addr, socklen_t *addrlen);
const char *message_type_name(uint32_t type);

#endif
