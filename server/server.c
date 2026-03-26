#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/protocol.h"
#include "../common/utils.h"
#include "../security/ssl_setup.h"
#include "include/server.h"

static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t next_session_id = 1;

static uint32_t allocate_session_id(void) {
    uint32_t session_id;

    pthread_mutex_lock(&session_mutex);
    session_id = next_session_id++;
    if (next_session_id == 0) {
        next_session_id = 1;
    }
    pthread_mutex_unlock(&session_mutex);
    return session_id;
}

static int create_data_socket(uint32_t *port_out) {
    int data_socket;
    struct sockaddr_in data_addr;
    socklen_t addr_len = sizeof(data_addr);

    data_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (data_socket < 0) {
        perror("socket");
        return -1;
    }

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = htons(0);

    if (bind(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("bind");
        close(data_socket);
        return -1;
    }

    if (getsockname(data_socket, (struct sockaddr *)&data_addr, &addr_len) < 0) {
        perror("getsockname");
        close(data_socket);
        return -1;
    }

    *port_out = (uint32_t)ntohs(data_addr.sin_port);
    return data_socket;
}

static void send_error_packet(int socket_fd, const struct sockaddr_in *client_addr,
                              socklen_t client_len, const char *message) {
    Packet packet;

    init_packet(&packet, MSG_TYPE_ERROR);
    strncpy(packet.filename, message, MAX_FILENAME_LENGTH);
    packet.filename[MAX_FILENAME_LENGTH] = '\0';
    send_packet(socket_fd, &packet, client_addr, client_len);
}

int start_server(const ServerConfig *config) {
    int control_socket;
    struct sockaddr_in server_addr;
    SSL_CTX *ssl_ctx;

    ssl_ctx = create_server_dtls_context(config->cert_path, config->key_path);
    if (ssl_ctx == NULL) {
        return EXIT_FAILURE;
    }

    control_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (control_socket < 0) {
        perror("socket");
        SSL_CTX_free(ssl_ctx);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config->port);

    if (bind(control_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(control_socket);
        SSL_CTX_free(ssl_ctx);
        return EXIT_FAILURE;
    }

    printf("[SERVER] UDP control socket listening on port %d\n", config->port);
    printf("[SERVER] Serving files from '%s'\n", config->storage_dir);
    printf("[SERVER] DTLS enabled for transfer sessions\n");

    while (1) {
        Packet request;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char path[MAX_PATH_LENGTH];
        uint32_t file_crc32;
        uint32_t file_size;
        uint32_t total_chunks;
        uint32_t accepted_seq;
        uint32_t data_port;
        int data_socket;
        TransferSession *session;
        Packet info;

        if (recv_packet(control_socket, &request, &client_addr, &client_len) < 0) {
            continue;
        }

        if (request.type != MSG_TYPE_REQUEST) {
            send_error_packet(control_socket, &client_addr, client_len,
                              "Expected REQUEST packet");
            continue;
        }

        if (!is_safe_filename(request.filename)) {
            send_error_packet(control_socket, &client_addr, client_len,
                              "Invalid filename");
            continue;
        }

        if (snprintf(path, sizeof(path), "%s/%s",
                     config->storage_dir, request.filename) >= (int)sizeof(path)) {
            send_error_packet(control_socket, &client_addr, client_len,
                              "Resolved path is too long");
            continue;
        }

        if (compute_file_crc32(path, &file_crc32, &file_size) != 0) {
            send_error_packet(control_socket, &client_addr, client_len,
                              "Requested file does not exist on the server");
            continue;
        }

        total_chunks = (file_size == 0)
            ? 0
            : (uint32_t)((file_size + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE);
        accepted_seq = request.seq_no;
        if (accepted_seq > total_chunks) {
            accepted_seq = total_chunks;
        }

        data_socket = create_data_socket(&data_port);
        if (data_socket < 0) {
            send_error_packet(control_socket, &client_addr, client_len,
                              "Failed to create transfer socket");
            continue;
        }

        session = (TransferSession *)calloc(1, sizeof(*session));
        if (session == NULL) {
            perror("calloc");
            close(data_socket);
            send_error_packet(control_socket, &client_addr, client_len,
                              "Out of memory");
            continue;
        }

        session->data_socket = data_socket;
        session->ssl_ctx = ssl_ctx;
        session->client_addr = client_addr;
        session->client_len = client_len;
        session->session_id = allocate_session_id();
        session->start_seq = accepted_seq;
        session->total_chunks = total_chunks;
        session->file_size = file_size;
        session->file_crc32 = file_crc32;
        session->window_size = WINDOW_SIZE;
        strncpy(session->storage_dir, config->storage_dir,
                sizeof(session->storage_dir) - 1);
        session->storage_dir[sizeof(session->storage_dir) - 1] = '\0';
        strncpy(session->filename, request.filename, MAX_FILENAME_LENGTH);
        session->filename[MAX_FILENAME_LENGTH] = '\0';

        if (spawn_detached_thread(handle_transfer_session, session) != 0) {
            close(data_socket);
            free(session);
            send_error_packet(control_socket, &client_addr, client_len,
                              "Failed to start transfer session");
            continue;
        }

        init_packet(&info, MSG_TYPE_SESSION_INFO);
        info.session_id = session->session_id;
        info.seq_no = accepted_seq;
        info.total_chunks = total_chunks;
        info.window_size = WINDOW_SIZE;
        info.checksum = file_crc32;
        info.file_size = file_size;
        info.port = data_port;
        strncpy(info.filename, request.filename, MAX_FILENAME_LENGTH);
        info.filename[MAX_FILENAME_LENGTH] = '\0';

        send_packet(control_socket, &info, &client_addr, client_len);
        printf("[SERVER] Prepared DTLS session %u for '%s' on port %u (resume chunk %u)\n",
               session->session_id, session->filename, data_port, accepted_seq);
    }

    close(control_socket);
    SSL_CTX_free(ssl_ctx);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    ServerConfig config;

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    config.port = DEFAULT_SERVER_PORT;
    strncpy(config.storage_dir, DEFAULT_STORAGE_DIR, sizeof(config.storage_dir) - 1);
    config.storage_dir[sizeof(config.storage_dir) - 1] = '\0';
    config.cert_path = DEFAULT_SERVER_CERT_PATH;
    config.key_path = DEFAULT_SERVER_KEY_PATH;

    if (argc >= 2) {
        config.port = atoi(argv[1]);
    }

    if (argc >= 3) {
        strncpy(config.storage_dir, argv[2], sizeof(config.storage_dir) - 1);
        config.storage_dir[sizeof(config.storage_dir) - 1] = '\0';
    }

    if (config.port <= 0 || config.port > 65535) {
        fprintf(stderr, "Usage: %s [port] [storage_dir]\n", argv[0]);
        return EXIT_FAILURE;
    }

    return start_server(&config);
}
