#ifndef SSL_SETUP_H
#define SSL_SETUP_H

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "../common/protocol.h"

SSL_CTX *create_server_dtls_context(const char *cert_path, const char *key_path);
SSL_CTX *create_client_dtls_context(const char *ca_cert_path);
SSL *accept_dtls_session(SSL_CTX *ctx, int socket_fd,
                         const struct sockaddr_in *peer_addr);
SSL *connect_dtls_session(SSL_CTX *ctx, int socket_fd,
                          const struct sockaddr_in *peer_addr);
int send_secure_packet(SSL *ssl, const Packet *packet);
int recv_secure_packet(SSL *ssl, Packet *packet);

#endif
