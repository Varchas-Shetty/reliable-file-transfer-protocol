#include <stdio.h>
#include <stdlib.h>

#include "../common/constants.h"
#include "include/client.h"

static void print_usage(const char *program_name) {
    fprintf(stderr,
            "Usage:\n"
            "  %s                       # interactive mode\n"
            "  %s <filename>\n"
            "  %s <host> <filename>\n"
            "  %s <host> <filename> <output_path>\n",
            program_name, program_name, program_name, program_name);
}

int main(int argc, char **argv) {
    ClientOptions options;

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    options.host = DEFAULT_SERVER_HOST;
    options.port = DEFAULT_SERVER_PORT;
    options.filename = NULL;
    options.output_path = NULL;
    options.ca_cert_path = DEFAULT_CA_CERT_PATH;

    if (argc == 1) {
        return run_client_interactive(options.host, options.port,
                                      options.ca_cert_path);
    }

    if (argc == 2) {
        options.filename = argv[1];
        return download_file(&options);
    }

    if (argc == 3) {
        options.host = argv[1];
        options.filename = argv[2];
        return download_file(&options);
    }

    if (argc == 4) {
        options.host = argv[1];
        options.filename = argv[2];
        options.output_path = argv[3];
        return download_file(&options);
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
