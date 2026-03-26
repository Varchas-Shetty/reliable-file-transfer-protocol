#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/protocol.h"
#include "../common/utils.h"
#include "../security/ssl_setup.h"
#include "include/client.h"

static int resolve_server_address(const char *host, int port,
                                  struct sockaddr_in *server_addr) {
    char port_string[16];
    struct addrinfo hints;
    struct addrinfo *results = NULL;
    struct addrinfo *current = NULL;
    int success = -1;

    snprintf(port_string, sizeof(port_string), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(host, port_string, &hints, &results) != 0) {
        return -1;
    }

    for (current = results; current != NULL; current = current->ai_next) {
        if (current->ai_addrlen == sizeof(*server_addr)) {
            memcpy(server_addr, current->ai_addr, sizeof(*server_addr));
            success = 0;
            break;
        }
    }

    freeaddrinfo(results);
    return success;
}

static int wait_for_packet(int socket_fd, Packet *packet,
                           struct sockaddr_in *peer_addr, socklen_t *peer_len,
                           int seconds, int usec) {
    fd_set read_fds;
    struct timeval timeout;
    int ready;

    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    timeout.tv_sec = seconds;
    timeout.tv_usec = usec;

    ready = select(socket_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ready <= 0) {
        return ready;
    }

    if (recv_packet(socket_fd, packet, peer_addr, peer_len) < 0) {
        return -1;
    }

    return 1;
}

static int send_ack(SSL *ssl, uint32_t session_id, uint32_t next_expected_seq) {
    Packet ack;

    init_packet(&ack, MSG_TYPE_ACK);
    ack.session_id = session_id;
    ack.ack_no = next_expected_seq;
    return send_secure_packet(ssl, &ack);
}

static size_t existing_file_size(const char *path) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }

    return (size_t)st.st_size;
}

static uint32_t initial_resume_seq_for_path(const char *path) {
    size_t current_size = existing_file_size(path);
    size_t aligned_size = aligned_resume_size(current_size);

    return (uint32_t)(aligned_size / MAX_PAYLOAD_SIZE);
}

static int prepare_output_file(const char *path, uint32_t server_file_size,
                               uint32_t *resume_seq_out) {
    size_t current_size = existing_file_size(path);
    size_t aligned_size;

    if (current_size > server_file_size) {
        current_size = 0;
        if (truncate_file_to_size(path, 0) != 0) {
            return -1;
        }
    }

    aligned_size = aligned_resume_size(current_size);
    if (aligned_size != current_size &&
        truncate_file_to_size(path, aligned_size) != 0) {
            return -1;
        }

    *resume_seq_out = (uint32_t)(aligned_size / MAX_PAYLOAD_SIZE);
    return 0;
}

int download_file(const ClientOptions *options) {
    int socket_fd = -1;
    struct sockaddr_in control_addr;
    struct sockaddr_in peer_addr;
    struct sockaddr_in transfer_addr;
    socklen_t peer_len = sizeof(peer_addr);
    Packet packet;
    FILE *output = NULL;
    char output_path[MAX_PATH_LENGTH];
    uint32_t requested_resume_seq;
    uint32_t resume_seq = 0;
    uint32_t expected_seq;
    uint32_t session_id;
    uint32_t total_chunks;
    uint32_t file_size;
    uint32_t file_crc32;
    int attempt;
    int idle_timeouts = 0;
    double start_time;
    double end_time;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;

    if (!is_safe_filename(options->filename)) {
        fprintf(stderr, "[CLIENT] Invalid filename '%s'\n", options->filename);
        return EXIT_FAILURE;
    }

    if (resolve_server_address(options->host, options->port, &control_addr) != 0) {
        fprintf(stderr, "[CLIENT] Failed to resolve host '%s'\n", options->host);
        return EXIT_FAILURE;
    }

    if (options->output_path != NULL) {
        strncpy(output_path, options->output_path, sizeof(output_path) - 1);
        output_path[sizeof(output_path) - 1] = '\0';
    } else {
        snprintf(output_path, sizeof(output_path), "downloaded_%s", options->filename);
    }

    requested_resume_seq = initial_resume_seq_for_path(output_path);

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    init_packet(&packet, MSG_TYPE_REQUEST);
    packet.seq_no = requested_resume_seq;
    strncpy(packet.filename, options->filename, MAX_FILENAME_LENGTH);
    packet.filename[MAX_FILENAME_LENGTH] = '\0';

    for (attempt = 0; attempt < CONTROL_RETRY_COUNT; ++attempt) {
        if (send_packet(socket_fd, &packet, &control_addr, sizeof(control_addr)) < 0) {
            close(socket_fd);
            return EXIT_FAILURE;
        }

        if (wait_for_packet(socket_fd, &packet, &peer_addr, &peer_len, 1, 0) == 1 &&
            (packet.type == MSG_TYPE_SESSION_INFO || packet.type == MSG_TYPE_ERROR)) {
            break;
        }
    }

    if (packet.type == MSG_TYPE_ERROR) {
        fprintf(stderr, "[CLIENT] Server error: %s\n", packet.filename);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (packet.type != MSG_TYPE_SESSION_INFO) {
        fprintf(stderr, "[CLIENT] Failed to receive session info from server\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    session_id = packet.session_id;
    total_chunks = packet.total_chunks;
    file_size = packet.file_size;
    file_crc32 = packet.checksum;
    transfer_addr = control_addr;
    transfer_addr.sin_port = htons((uint16_t)packet.port);

    if (prepare_output_file(output_path, file_size, &resume_seq) != 0) {
        perror("[CLIENT] prepare_output_file");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (resume_seq > packet.seq_no) {
        resume_seq = packet.seq_no;
        if (truncate_file_to_size(output_path,
                                  (size_t)resume_seq * MAX_PAYLOAD_SIZE) != 0) {
            perror("[CLIENT] truncate");
            close(socket_fd);
            return EXIT_FAILURE;
        }
    }

    output = fopen(output_path, "rb+");
    if (output == NULL) {
        output = fopen(output_path, "wb+");
    }
    if (output == NULL) {
        perror("[CLIENT] fopen");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (connect(socket_fd, (struct sockaddr *)&transfer_addr, sizeof(transfer_addr)) != 0) {
        perror("[CLIENT] connect");
        fclose(output);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    ctx = create_client_dtls_context(options->ca_cert_path);
    if (ctx == NULL) {
        fclose(output);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    ssl = connect_dtls_session(ctx, socket_fd, &transfer_addr);
    if (ssl == NULL) {
        SSL_CTX_free(ctx);
        fclose(output);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    init_packet(&packet, MSG_TYPE_START);
    packet.session_id = session_id;
    packet.seq_no = resume_seq;
    if (send_secure_packet(ssl, &packet) != 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        fclose(output);
        close(socket_fd);
        return EXIT_FAILURE;
    }

    expected_seq = resume_seq;
    printf("[CLIENT] Starting secure UDP download of '%s' from %s:%d\n",
           options->filename, options->host, options->port);
    if (requested_resume_seq > 0) {
        printf("[CLIENT] Requested resume from chunk %u\n", requested_resume_seq);
    }
    if (expected_seq > 0) {
        printf("[CLIENT] Continuing from chunk %u\n", expected_seq);
    }

    start_time = now_seconds();

    while (1) {
        fd_set read_fds;
        struct timeval timeout;
        int ready;

        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        timeout.tv_sec = CLIENT_IDLE_TIMEOUT_SEC;
        timeout.tv_usec = 0;

        ready = select(socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready == 0) {
            idle_timeouts++;
            if (idle_timeouts >= MAX_RETRIES) {
                fprintf(stderr, "[CLIENT] Transfer timed out waiting for data\n");
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                fclose(output);
                close(socket_fd);
                return EXIT_FAILURE;
            }
            send_ack(ssl, session_id, expected_seq);
            continue;
        }

        if (ready < 0) {
            fprintf(stderr, "[CLIENT] Failed while waiting for secure UDP packets\n");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            fclose(output);
            close(socket_fd);
            return EXIT_FAILURE;
        }

        idle_timeouts = 0;
        if (recv_secure_packet(ssl, &packet) != 0) {
            fprintf(stderr, "[CLIENT] Failed while receiving secure UDP packets\n");
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            fclose(output);
            close(socket_fd);
            return EXIT_FAILURE;
        }

        if (packet.session_id != session_id) {
            continue;
        }

        if (packet.type == MSG_TYPE_ERROR) {
            fprintf(stderr, "[CLIENT] Server error: %s\n", packet.filename);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            fclose(output);
            close(socket_fd);
            return EXIT_FAILURE;
        }

        if (packet.type == MSG_TYPE_DATA) {
            if (packet.checksum != crc32_buffer(packet.data, packet.data_size)) {
                send_ack(ssl, session_id, expected_seq);
                continue;
            }

            if (packet.seq_no == expected_seq) {
                long offset = (long)packet.seq_no * MAX_PAYLOAD_SIZE;
                if (fseek(output, offset, SEEK_SET) != 0 ||
                    fwrite(packet.data, 1, packet.data_size, output) != packet.data_size) {
                    perror("[CLIENT] fwrite");
                    SSL_shutdown(ssl);
                    SSL_free(ssl);
                    SSL_CTX_free(ctx);
                    fclose(output);
                    close(socket_fd);
                    return EXIT_FAILURE;
                }
                fflush(output);
                expected_seq++;
                printf("[CLIENT] Received chunk %u/%u\r",
                       expected_seq, total_chunks);
            }

            if (send_ack(ssl, session_id, expected_seq) != 0) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                fclose(output);
                close(socket_fd);
                return EXIT_FAILURE;
            }
            continue;
        }

        if (packet.type == MSG_TYPE_END) {
            uint32_t local_crc32;
            uint32_t local_size;

            if (compute_file_crc32(output_path, &local_crc32, &local_size) != 0) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                fclose(output);
                close(socket_fd);
                return EXIT_FAILURE;
            }

            if (expected_seq != total_chunks ||
                local_size != file_size ||
                local_crc32 != file_crc32) {
                fprintf(stderr, "\n[CLIENT] Integrity check failed after transfer\n");
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                fclose(output);
                close(socket_fd);
                return EXIT_FAILURE;
            }

            send_ack(ssl, session_id, total_chunks);
            break;
        }
    }

    end_time = now_seconds();
    printf("\n[CLIENT] Transfer complete in %.2f seconds\n", end_time - start_time);
    printf("[CLIENT] Integrity verified with CRC32 0x%08X\n", file_crc32);
    printf("[CLIENT] Saved as %s\n", output_path);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    fclose(output);
    close(socket_fd);
    return EXIT_SUCCESS;
}

int run_client_interactive(const char *host, int port, const char *ca_cert_path) {
    char filename[MAX_FILENAME_LENGTH + 2];

    while (1) {
        ClientOptions options;

        printf("\n===== Secure UDP File Client =====\n");
        printf("Enter filename to download (or 'exit' to quit): ");

        if (read_stdin_line(filename, sizeof(filename)) != 0) {
            return EXIT_FAILURE;
        }

        if (strcmp(filename, "exit") == 0) {
            break;
        }

        if (filename[0] == '\0') {
            continue;
        }

        options.host = host;
        options.port = port;
        options.filename = filename;
        options.output_path = NULL;
        options.ca_cert_path = ca_cert_path;

        if (download_file(&options) != EXIT_SUCCESS) {
            fprintf(stderr, "[CLIENT] Download failed for '%s'\n", filename);
        }
    }

    return EXIT_SUCCESS;
}
