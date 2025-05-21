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
        fprintf(stderr, "Uso: %s <seu_id (1-999)> <ip_servidor> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int uid = atoi(argv[1]);
    if (uid < 1 || uid > 999) {
        fprintf(stderr, "ID de cliente exibidor deve ser entre 1 e 999.\n");
        exit(EXIT_FAILURE);
    }

    char *ip = argv[2];
    int port = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in server_addr;

    // Criar socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die("socket");
    }

    // Endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        die("inet_pton");
    }

    // Conectar
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die("connect");
    }

    // Enviar OI
    msg_t msg;
    msg.type = htons(MSG_OI);
    msg.orig_uid = htons(uid);
    msg.dest_uid = 0;
    msg.text_len = 0;
    memset(msg.text, 0, sizeof(msg.text));

    if (send(sockfd, &msg, sizeof(msg_t), 0) <= 0) die("send OI");

    // Esperar resposta OI
    if (recv(sockfd, &msg, sizeof(msg_t), 0) <= 0) die("recv OI");

    if (ntohs(msg.type) != MSG_OI) {
        fprintf(stderr, "Erro: servidor rejeitou a conexão.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor como exibidor (ID %d). Aguardando mensagens...\n", uid);

    // Loop de recepção
    while (1) {
        int ret = recv(sockfd, &msg, sizeof(msg_t), 0);
        if (ret <= 0) {
            printf("Conexão encerrada.\n");
            break;
        }

        msg.type = ntohs(msg.type);
        msg.orig_uid = ntohs(msg.orig_uid);
        msg.dest_uid = ntohs(msg.dest_uid);
        msg.text_len = ntohs(msg.text_len);
        msg.text[msg.text_len] = '\0'; // Garantir término da string

        if (msg.type == MSG_MSG) {
            if (msg.dest_uid == 0)
                printf("[Broadcast] Mensagem de %d: %s\n", msg.orig_uid, msg.text);
            else
                printf("[Privado] Mensagem de %d: %s\n", msg.orig_uid, msg.text);
        }
        else if (msg.type == MSG_TCHAU) {
            printf("Servidor enviou TCHAU. Encerrando.\n");
            break;
        }
        else {
            printf("Recebido tipo desconhecido (%d)\n", msg.type);
        }
    }

    close(sockfd);
    return 0;
}
