#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ssl_setup.h"

static SSL_CTX *create_context(const SSL_METHOD *method) {
    SSL_CTX *ctx;

    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();

    ctx = SSL_CTX_new(method);
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    SSL_CTX_set_read_ahead(ctx, 1);
    return ctx;
}

static SSL *create_ssl_for_socket(SSL_CTX *ctx, int socket_fd,
                                  const struct sockaddr_in *peer_addr) {
    SSL *ssl;
    BIO *bio;

    ssl = SSL_new(ctx);
    if (ssl == NULL) {
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    bio = BIO_new_dgram(socket_fd, BIO_NOCLOSE);
    if (bio == NULL) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    if (peer_addr != NULL) {
        BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, (void *)peer_addr);
    }

    SSL_set_bio(ssl, bio, bio);
    return ssl;
}

SSL_CTX *create_server_dtls_context(const char *cert_path, const char *key_path) {
    SSL_CTX *ctx = create_context(DTLS_server_method());

    if (ctx == NULL) {
        return NULL;
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_check_private_key(ctx) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

SSL_CTX *create_client_dtls_context(const char *ca_cert_path) {
    SSL_CTX *ctx = create_context(DTLS_client_method());

    if (ctx == NULL) {
        return NULL;
    }

    if (SSL_CTX_load_verify_locations(ctx, ca_cert_path, NULL) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    return ctx;
}

SSL *accept_dtls_session(SSL_CTX *ctx, int socket_fd,
                         const struct sockaddr_in *peer_addr) {
    SSL *ssl = create_ssl_for_socket(ctx, socket_fd, peer_addr);

    if (ssl == NULL) {
        return NULL;
    }

    SSL_set_accept_state(ssl);
    if (SSL_accept(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    return ssl;
}

SSL *connect_dtls_session(SSL_CTX *ctx, int socket_fd,
                          const struct sockaddr_in *peer_addr) {
    SSL *ssl = create_ssl_for_socket(ctx, socket_fd, peer_addr);

    if (ssl == NULL) {
        return NULL;
    }

    SSL_set_connect_state(ssl);
    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    return ssl;
}

int send_secure_packet(SSL *ssl, const Packet *packet) {
    Packet network_packet = *packet;
    int written;

    packet_host_to_network(&network_packet);
    written = SSL_write(ssl, &network_packet, (int)sizeof(network_packet));
    if (written != (int)sizeof(network_packet)) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    return 0;
}

int recv_secure_packet(SSL *ssl, Packet *packet) {
    int received = SSL_read(ssl, packet, (int)sizeof(*packet));

    if (received != (int)sizeof(*packet)) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    return packet_network_to_host(packet);
}
