#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include "include/server.h"
#include "../common/constants.h"
#include "../common/protocol.h"

// Ignore child exit to prevent zombies
void handle_sigchld(int sig) {
    (void)sig;
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void send_file(int server_fd, struct sockaddr_in *client_addr, socklen_t addr_len, char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("File open failed");
        Message err;
        err.type = MSG_TYPE_FILE_DATA;
        err.seq_no = -1;
        err.data_size = snprintf(err.data, CHUNK_SIZE, "File not found");
        sendto(server_fd, &err, sizeof(err), 0,
               (struct sockaddr *)client_addr, addr_len);
        return;
    }

    Message msg, ack;
    int seq = 0;
    int retries;
    time_t start_time = time(NULL);

    while (1) {
        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_TYPE_FILE_DATA;
        msg.seq_no = seq;
        msg.data_size = fread(msg.data, 1, CHUNK_SIZE, fp);

        if (msg.data_size <= 0) break;

        retries = 0;
        while (retries < MAX_RETRIES) {
            sendto(server_fd, &msg, sizeof(msg), 0,
                   (struct sockaddr *)client_addr, addr_len);

            // Set timeout for ACK
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_fd, &readfds);
            struct timeval tv = {TIMEOUT_SEC, 0};

            int sel = select(server_fd + 1, &readfds, NULL, NULL, &tv);
            if (sel > 0 && FD_ISSET(server_fd, &readfds)) {
                socklen_t len = addr_len;
                recvfrom(server_fd, &ack, sizeof(ack), 0,
                         (struct sockaddr *)client_addr, &len);
                if (ack.type == MSG_TYPE_ACK && ack.seq_no == seq) {
                    // Correct ACK received
                    break;
                }
            }
            retries++;
        }

        if (retries == MAX_RETRIES) {
            printf("Client unresponsive, stopping transfer.\n");
            fclose(fp);
            return;
        }

        seq++;
    }

    // Send END message
    Message end_msg;
    end_msg.type = MSG_TYPE_END;
    end_msg.seq_no = seq;
    end_msg.data_size = 0;
    sendto(server_fd, &end_msg, sizeof(end_msg), 0,
           (struct sockaddr *)client_addr, addr_len);

    fclose(fp);

    time_t end_time = time(NULL);
    printf("File transfer complete for client %s:%d. Time taken: %ld seconds\n",
           inet_ntoa(client_addr->sin_addr),
           ntohs(client_addr->sin_port),
           end_time - start_time);
}

void start_server() {
    signal(SIGCHLD, handle_sigchld); // prevent zombie processes

    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    Message msg;

    // Create UDP socket
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("UDP File Server running on port %d...\n", SERVER_PORT);

    while (1) {
        memset(&msg, 0, sizeof(msg));

        int recv_len = recvfrom(server_fd, &msg, sizeof(msg), 0,
                                (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            continue;
        }

        // Fork for concurrent client handling
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            continue;
        }

        if (pid == 0) {
            // CHILD process handles this client
            // Send ACK for request
            Message ack;
            ack.type = MSG_TYPE_ACK;
            ack.seq_no = msg.seq_no;
            ack.data_size = 0;
            sendto(server_fd, &ack, sizeof(ack), 0,
                   (struct sockaddr *)&client_addr, addr_len);

            if (msg.type == MSG_TYPE_FILE_REQUEST) {
                printf("Client requested file: %s\n", msg.data);
                send_file(server_fd, &client_addr, addr_len, msg.data);
            } else {
                printf("Invalid message type received.\n");
            }
            exit(0); // child done
        }

        // Parent continues listening
    }

    close(server_fd);
}

int main() {
    start_server();
    return 0;
}