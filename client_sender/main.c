#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/protocol.h"

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <seu_id (1001-1999)> <ip_servidor> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int uid = atoi(argv[1]);
    if (uid < 1001 || uid > 1999) {
        fprintf(stderr, "ID do emissor deve ser entre 1001 e 1999.\n");
        exit(EXIT_FAILURE);
    }

    char *ip = argv[2];
    int port = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in server_addr;

    // Criar socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket");

    // Endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
        die("inet_pton");

    // Conectar ao servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die("connect");

    // Enviar mensagem OI
    msg_t msg;
    msg.type = htons(MSG_OI);
    msg.orig_uid = htons(uid);
    msg.dest_uid = 0;
    msg.text_len = 0;
    memset(msg.text, 0, sizeof(msg.text));

    if (send(sockfd, &msg, sizeof(msg_t), 0) <= 0)
        die("send OI");

    // Receber resposta OI
    if (recv(sockfd, &msg, sizeof(msg_t), 0) <= 0)
        die("recv OI");

    if (ntohs(msg.type) != MSG_OI) {
        fprintf(stderr, "Servidor rejeitou a conexão.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor como emissor (ID %d).\n", uid);
    printf("Digite <dest_uid> <mensagem>. Digite TCHAU para sair.\n");

    // Loop principal
    while (1) {
        printf("> ");
        fflush(stdout);

        char buffer[256];
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;

        // Verificar comando TCHAU
        if (strncmp(buffer, "TCHAU", 5) == 0 || strncmp(buffer, "tchau", 5) == 0) {
            msg_t bye_msg;
            bye_msg.type = htons(MSG_TCHAU);
            bye_msg.orig_uid = htons(uid);
            bye_msg.dest_uid = 0;
            bye_msg.text_len = 0;
            memset(bye_msg.text, 0, sizeof(bye_msg.text));

            send(sockfd, &bye_msg, sizeof(bye_msg), 0);
            printf("Desconectado.\n");
            break;
        }

        int dest_uid;
        char text[141];

        if (sscanf(buffer, "%d %[^\n]", &dest_uid, text) != 2) {
            printf("Formato inválido. Use: <dest_uid> <mensagem>\n");
            continue;
        }

        size_t len = strlen(text);
        if (len >= 141) {
            printf("Mensagem muito longa! Use até 140 caracteres.\n");
            continue;
        }

        msg_t send_msg;
        send_msg.type = htons(MSG_MSG);
        send_msg.orig_uid = htons(uid);
        send_msg.dest_uid = htons(dest_uid);
        send_msg.text_len = htons(len);
        memset(send_msg.text, 0, sizeof(send_msg.text));
        strncpy((char *)send_msg.text, text, len);
        send_msg.text[len] = '\0';

        if (send(sockfd, &send_msg, sizeof(msg_t), 0) <= 0) {
            perror("Erro ao enviar");
            break;
        }
    }

    close(sockfd);
    return 0;
}
