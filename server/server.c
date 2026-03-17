#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../security/ssl_setup.h"

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int server_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
} client_request;

void *handle_client(void *arg) {

    client_request *req = (client_request *)arg;

    printf("Client connected: %s\n",
           inet_ntoa(req->client_addr.sin_addr));

    int seq;
    sscanf(req->buffer, "%d", &seq);

    printf("Packet received | Seq = %d | Data = %s\n", seq, req->buffer);

    char ack[] = "ACK";

    printf("Sending ACK for packet %d\n", seq);

    sendto(req->server_fd,
           ack,
           strlen(ack),
           0,
           (struct sockaddr *)&req->client_addr,
           req->addr_len);

    free(req);

    pthread_exit(NULL);
}
int main() {

    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    /* ===== SSL INITIALIZATION ===== */

    SSL_CTX *ctx;

    ctx = create_ssl_context();
    configure_ssl_context(ctx);

    printf("SSL initialized successfully\n");

    /* ===== CREATE UDP SOCKET ===== */

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP File Server running on port %d...\n", PORT);

    while (1) {

        client_request *req = malloc(sizeof(client_request));
        req->server_fd = server_fd;
        req->addr_len = addr_len;

        int n = recvfrom(server_fd,
                         req->buffer,
                         BUFFER_SIZE,
                         0,
                         (struct sockaddr *)&req->client_addr,
                         &req->addr_len);

        if (n < 0) {
            perror("Receive failed");
            free(req);
            continue;
        }

        req->buffer[n] = '\0';

        pthread_t thread;

        pthread_create(&thread,
                       NULL,
                       handle_client,
                       (void *)req);

        pthread_detach(thread);
    }

    close(server_fd);
    SSL_CTX_free(ctx);
srand(time(NULL));
    return 0;
}
