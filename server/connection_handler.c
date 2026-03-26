#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/protocol.h"
#include "../common/utils.h"
#include "../security/ssl_setup.h"
#include "include/server.h"

static int wait_for_ack(SSL *ssl, uint32_t *ack_no_out) {
    fd_set read_fds;
    struct timeval timeout;
    int socket_fd = SSL_get_fd(ssl);
    Packet packet;
    int ready;

    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = ACK_TIMEOUT_USEC;

    ready = select(socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ready <= 0) {
        return ready;
    }

    if (recv_secure_packet(ssl, &packet) != 0) {
        return -1;
    }

    if (packet.type != MSG_TYPE_ACK) {
        return -1;
    }

    *ack_no_out = packet.ack_no;
    return 1;
}

static int send_chunk(FILE *fp, const TransferSession *session, SSL *ssl, uint32_t seq_no) {
    Packet chunk;
    long offset = (long)seq_no * MAX_PAYLOAD_SIZE;
    size_t bytes_read;

    if (fseek(fp, offset, SEEK_SET) != 0) {
        return -1;
    }

    init_packet(&chunk, MSG_TYPE_DATA);
    chunk.session_id = session->session_id;
    chunk.seq_no = seq_no;
    chunk.total_chunks = session->total_chunks;
    chunk.file_size = session->file_size;

    bytes_read = fread(chunk.data, 1, sizeof(chunk.data), fp);
    if (bytes_read == 0 && ferror(fp)) {
        return -1;
    }

    chunk.data_size = (uint32_t)bytes_read;
    chunk.checksum = crc32_buffer(chunk.data, bytes_read);
    return send_secure_packet(ssl, &chunk);
}

static int resend_window(FILE *fp, const TransferSession *session, SSL *ssl,
                         uint32_t base_seq, uint32_t next_seq) {
    uint32_t seq_no;

    for (seq_no = base_seq; seq_no < next_seq; ++seq_no) {
        if (send_chunk(fp, session, ssl, seq_no) != 0) {
            return -1;
        }
    }

    return 0;
}

static int wait_for_session_start(SSL *ssl, uint32_t session_id) {
    Packet packet;

    if (recv_secure_packet(ssl, &packet) != 0) {
        return -1;
    }

    if (packet.type != MSG_TYPE_START || packet.session_id != session_id) {
        return -1;
    }

    return 0;
}

static int send_end_packet(const TransferSession *session, SSL *ssl) {
    Packet packet;

    init_packet(&packet, MSG_TYPE_END);
    packet.session_id = session->session_id;
    packet.total_chunks = session->total_chunks;
    packet.file_size = session->file_size;
    packet.checksum = session->file_crc32;
    return send_secure_packet(ssl, &packet);
}

void *handle_transfer_session(void *arg) {
    TransferSession *session = (TransferSession *)arg;
    char path[MAX_PATH_LENGTH];
    FILE *fp = NULL;
    uint32_t base_seq;
    uint32_t next_seq;
    int end_attempt;
    SSL *ssl = NULL;

    if (snprintf(path, sizeof(path), "%s/%s",
                 session->storage_dir, session->filename) >= (int)sizeof(path)) {
        close(session->data_socket);
        free(session);
        return NULL;
    }

    if (connect(session->data_socket, (struct sockaddr *)&session->client_addr,
                session->client_len) != 0) {
        close(session->data_socket);
        free(session);
        return NULL;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        close(session->data_socket);
        free(session);
        return NULL;
    }

    ssl = accept_dtls_session(session->ssl_ctx, session->data_socket,
                              &session->client_addr);
    if (ssl == NULL) {
        printf("[SERVER] DTLS handshake failed for session %u\n", session->session_id);
        fclose(fp);
        close(session->data_socket);
        free(session);
        return NULL;
    }

    if (wait_for_session_start(ssl, session->session_id) != 0) {
        printf("[SERVER] Session %u failed to start for '%s'\n",
               session->session_id, session->filename);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        fclose(fp);
        close(session->data_socket);
        free(session);
        return NULL;
    }

    printf("[SERVER] DTLS session %u started for '%s' from chunk %u\n",
           session->session_id, session->filename, session->start_seq);

    base_seq = session->start_seq;
    next_seq = session->start_seq;

    while (base_seq < session->total_chunks) {
        uint32_t ack_no;
        int ack_status;

        while (next_seq < base_seq + session->window_size &&
               next_seq < session->total_chunks) {
            if (send_chunk(fp, session, ssl, next_seq) != 0) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                fclose(fp);
                close(session->data_socket);
                free(session);
                return NULL;
            }
            next_seq++;
        }

        ack_status = wait_for_ack(ssl, &ack_no);
        if (ack_status == 1) {
            if (ack_no > base_seq) {
                base_seq = ack_no;
            }
            continue;
        }

        if (ack_status == 0) {
            if (resend_window(fp, session, ssl, base_seq, next_seq) != 0) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                fclose(fp);
                close(session->data_socket);
                free(session);
                return NULL;
            }
            continue;
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        fclose(fp);
        close(session->data_socket);
        free(session);
        return NULL;
    }

    for (end_attempt = 0; end_attempt < MAX_RETRIES; ++end_attempt) {
        uint32_t ack_no;
        int ack_status;

        if (send_end_packet(session, ssl) != 0) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            fclose(fp);
            close(session->data_socket);
            free(session);
            return NULL;
        }

        ack_status = wait_for_ack(ssl, &ack_no);
        if (ack_status == 1 && ack_no == session->total_chunks) {
            printf("[SERVER] DTLS session %u completed for '%s'\n",
                   session->session_id, session->filename);
            break;
        }
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    fclose(fp);
    close(session->data_socket);
    free(session);
    return NULL;
}
