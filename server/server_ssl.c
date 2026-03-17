#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../security/ssl_setup.h"

void initialize_ssl()
{
    SSL_CTX *ctx;

    ctx = create_ssl_context();
    configure_ssl_context(ctx);

    printf("SSL context initialized successfully\n");
}
