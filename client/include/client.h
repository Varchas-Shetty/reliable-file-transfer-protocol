#ifndef CLIENT_H
#define CLIENT_H

typedef struct {
    const char *host;
    int port;
    const char *filename;
    const char *output_path;
    const char *ca_cert_path;
} ClientOptions;

int run_client_interactive(const char *host, int port, const char *ca_cert_path);
int download_file(const ClientOptions *options);

#endif
