#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <netinet/in.h>
#include <openssl/ssl.h>

#include "../../common/constants.h"

typedef struct {
    int port;
    char storage_dir[MAX_PATH_LENGTH];
    const char *cert_path;
    const char *key_path;
} ServerConfig;

typedef struct {
    int data_socket;
    SSL_CTX *ssl_ctx;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    uint32_t session_id;
    uint32_t start_seq;
    uint32_t total_chunks;
    uint32_t file_size;
    uint32_t file_crc32;
    uint32_t window_size;
    char storage_dir[MAX_PATH_LENGTH];
    char filename[MAX_FILENAME_LENGTH + 1];
} TransferSession;

int start_server(const ServerConfig *config);
void *handle_transfer_session(void *arg);
int spawn_detached_thread(void *(*routine)(void *), void *arg);

#endif
