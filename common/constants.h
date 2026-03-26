#ifndef CONSTANTS_H
#define CONSTANTS_H

#define DEFAULT_SERVER_HOST "127.0.0.1"
#define DEFAULT_SERVER_PORT 8080

#define MAX_PAYLOAD_SIZE 1024
#define MAX_FILENAME_LENGTH 255
#define MAX_PATH_LENGTH 512

#define MAX_RETRIES 5
#define WINDOW_SIZE 8
#define CONTROL_RETRY_COUNT 3
#define ACK_TIMEOUT_USEC 300000
#define SESSION_START_TIMEOUT_SEC 5
#define CLIENT_IDLE_TIMEOUT_SEC 10

#define DEFAULT_STORAGE_DIR "server"
#define DEFAULT_SERVER_CERT_PATH "security/certs/server.crt"
#define DEFAULT_SERVER_KEY_PATH "security/certs/server.key"
#define DEFAULT_CA_CERT_PATH "security/certs/server.crt"

#endif
