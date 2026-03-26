#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

#include "protocol.h"

void packet_host_to_network(Packet *packet) {
    packet->type = htonl(packet->type);
    packet->session_id = htonl(packet->session_id);
    packet->seq_no = htonl(packet->seq_no);
    packet->ack_no = htonl(packet->ack_no);
    packet->total_chunks = htonl(packet->total_chunks);
    packet->data_size = htonl(packet->data_size);
    packet->window_size = htonl(packet->window_size);
    packet->checksum = htonl(packet->checksum);
    packet->file_size = htonl(packet->file_size);
    packet->port = htonl(packet->port);
}

int packet_network_to_host(Packet *packet) {
    packet->type = ntohl(packet->type);
    packet->session_id = ntohl(packet->session_id);
    packet->seq_no = ntohl(packet->seq_no);
    packet->ack_no = ntohl(packet->ack_no);
    packet->total_chunks = ntohl(packet->total_chunks);
    packet->data_size = ntohl(packet->data_size);
    packet->window_size = ntohl(packet->window_size);
    packet->checksum = ntohl(packet->checksum);
    packet->file_size = ntohl(packet->file_size);
    packet->port = ntohl(packet->port);
    packet->filename[MAX_FILENAME_LENGTH] = '\0';

    if (packet->data_size > MAX_PAYLOAD_SIZE) {
        return -1;
    }

    return 0;
}

void init_packet(Packet *packet, uint32_t type) {
    memset(packet, 0, sizeof(*packet));
    packet->type = type;
}

ssize_t send_packet(int sockfd, const Packet *packet,
                    const struct sockaddr_in *addr, socklen_t addrlen) {
    Packet network_packet = *packet;

    packet_host_to_network(&network_packet);
    return sendto(sockfd, &network_packet, sizeof(network_packet), 0,
                  (const struct sockaddr *)addr, addrlen);
}

ssize_t recv_packet(int sockfd, Packet *packet,
                    struct sockaddr_in *addr, socklen_t *addrlen) {
    ssize_t received = recvfrom(sockfd, packet, sizeof(*packet), 0,
                                (struct sockaddr *)addr, addrlen);

    if (received >= (ssize_t)sizeof(*packet) &&
        packet_network_to_host(packet) != 0) {
        return -1;
    }

    return received;
}

const char *message_type_name(uint32_t type) {
    switch (type) {
        case MSG_TYPE_REQUEST:
            return "REQUEST";
        case MSG_TYPE_SESSION_INFO:
            return "SESSION_INFO";
        case MSG_TYPE_START:
            return "START";
        case MSG_TYPE_DATA:
            return "DATA";
        case MSG_TYPE_ACK:
            return "ACK";
        case MSG_TYPE_END:
            return "END";
        case MSG_TYPE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
