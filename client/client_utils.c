#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "include/client.h"
#include "../common/protocol.h"
#include "../common/constants.h"

void start_client() {
    int sock;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    Message msg, response;

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (1) {
        char filename[100];

        printf("\n===== FILE CLIENT =====\n");
        printf("Enter filename to download\n");
        printf("Type 'exit' to quit\n");
        printf("Enter: ");

        scanf("%s", filename);

        if (strcmp(filename, "exit") == 0)
            break;

        // Create request
        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_TYPE_FILE_REQUEST;
        msg.seq_no = 0;
        strcpy(msg.data, filename);
        msg.data_size = strlen(filename);

        sendto(sock, &msg, sizeof(msg), 0,
               (struct sockaddr*)&server_addr, addr_len);

        printf("[CLIENT] Requesting file: %s\n", filename);

        // Receive ACK
        recvfrom(sock, &response, sizeof(response), 0,
                 (struct sockaddr*)&server_addr, &addr_len);

        if (response.type == MSG_TYPE_ACK) {
            printf("[CLIENT] ACK received\n");
        }

        // Open file
        char output_file[150];
        sprintf(output_file, "downloaded_%s", filename);

        FILE *fp = fopen(output_file, "wb");
        if (!fp) {
            printf("File creation failed\n");
            continue;
        }

        int total_bytes = 0;

        // Receive file
        while (1) {
            recvfrom(sock, &response, sizeof(response), 0,
                     (struct sockaddr*)&server_addr, &addr_len);

            if (response.type == MSG_TYPE_FILE_DATA) {

                if (response.seq_no == -1) {
                    printf("Error: %s\n", response.data);
                    break;
                }

                fwrite(response.data, 1, response.data_size, fp);

                total_bytes += response.data_size;
                printf("Received: %d bytes\r", total_bytes);

                // Send ACK
                Message ack;
                ack.type = MSG_TYPE_ACK;
                ack.seq_no = response.seq_no;
                ack.data_size = 0;

                sendto(sock, &ack, sizeof(ack), 0,
                       (struct sockaddr*)&server_addr, addr_len);
            }
            else if (response.type == MSG_TYPE_END) {
                printf("\n[CLIENT] File received successfully!\n");
                printf("[CLIENT] Saved as %s\n", output_file);
                break;
            }
        }

        fclose(fp);
    }

    close(sock);
}