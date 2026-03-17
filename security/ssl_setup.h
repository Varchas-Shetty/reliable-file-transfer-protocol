#ifndef SSL_SETUP_H
#define SSL_SETUP_H

#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX* create_ssl_context();
void configure_ssl_context(SSL_CTX *ctx);

#endif
