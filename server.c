#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9090
#define BUF_SIZE 1024

void send_welcome(int client_fd)
{
    const char *msg =
        "Hi,\n"
        "Please Enter any two numbers or type EXIT to exit\n";

    send(client_fd, msg, strlen(msg), 0);
}

int main()
{
    int server_fd, client_fd = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    fd_set readfds;
    char buffer[BUF_SIZE];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Listen
    listen(server_fd, 1);

    printf("Server started on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        if (client_fd > 0)
            FD_SET(client_fd, &readfds);

        int maxfd = (server_fd > client_fd ? server_fd : client_fd) + 1;

        select(maxfd, &readfds, NULL, NULL, NULL);

        // New connection
        if (FD_ISSET(server_fd, &readfds)) {

            if (client_fd > 0) {
                printf("Server busy. Rejecting new client.\n");
                int temp = accept(server_fd, NULL, NULL);
                close(temp);
            } else {
                client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
                printf("Client connected.\n");
                send_welcome(client_fd);
            }
        }

        // Client data
        if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) {

            memset(buffer, 0, BUF_SIZE);
            int bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);

            if (bytes <= 0) {
                printf("Client disconnected.\n");
                close(client_fd);
                client_fd = -1;
                continue;
            }

            buffer[strcspn(buffer, "\r\n")] = 0; // remove newline

            if (strcasecmp(buffer, "EXIT") == 0) {
                printf("Client requested exit.\n");
                close(client_fd);
                client_fd = -1;
                continue;
            }

            int a, b;
            if (sscanf(buffer, "%d,%d", &a, &b) != 2) {
                const char *err = "Invalid format. Use: num1,num2\n";
                send(client_fd, err, strlen(err), 0);
                send_welcome(client_fd);
                continue;
            }

            char reply[BUF_SIZE];
            snprintf(reply, sizeof(reply),
                "Numbers : %d,%d\n"
                "Sum: %d + %d = %d\n"
                "Sub: %d - %d = %d\n"
                "Div: %d / %d = %d\n"
                "Mul : %d x %d = %d\n\n",
                a, b,
                a, b, a + b,
                a, b, a - b,
                a, b, b != 0 ? a / b : 0,
                a, b, a * b);

            send(client_fd, reply, strlen(reply), 0);
            send_welcome(client_fd);
        }
    }

    close(server_fd);
    return 0;
}
