#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../../include/protocol.h"

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) die("socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(server_fd, 10) < 0) die("listen");

    printf("Servidor escutando na porta %d...\n", port);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&cli_addr, &cli_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Cliente conectado\n");

        msg_t msg;
        ssize_t n = recv(client_fd, &msg, sizeof(msg_t), 0);
        if (n <= 0) {
            perror("recv");
            close(client_fd);
            continue;
        }

        unsigned short type = ntohs(msg.type);
        unsigned short uid = ntohs(msg.orig_uid);
        printf("Recebido tipo %d de cliente %d\n", type, uid);

        if (type == MSG_OI) {
            // Envia de volta a mesma mensagem (tipo OI)
            send(client_fd, &msg, sizeof(msg_t), 0);
            printf("Respondido com OI\n");
        }

        close(client_fd);
        printf("Conexão encerrada\n");
    }

    close(server_fd);
    return 0;
}
