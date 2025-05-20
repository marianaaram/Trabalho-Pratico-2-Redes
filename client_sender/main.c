#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../../include/protocol.h"

#define SERVER_RESPONSE_TIMEOUT 5 // Timeout para resposta do servidor

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[]) { 
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <seu_id> <ip_servidor> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int uid = atoi(argv[1]);
    char *ip = argv[2];
    int port = atoi(argv[3]);

    if (uid < 1001 || uid > 1999) {
        fprintf(stderr, "ID de cliente inválido (deve ser entre 1001 e 1999)\n");
        exit(EXIT_FAILURE);
    }

    // Criar socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die("socket");

    // Configurar endereço
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        die("inet_pton");
    }

    // Conectar ao servidor
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        die("connect");
    }

    // Enviar mensagem OI
    msg_t msg;
    msg.type = htons(MSG_OI);
    msg.orig_uid = htons(uid);
    msg.dest_uid = 0;
    msg.text_len = 0;
    memset(msg.text, 0, sizeof(msg.text));

    if (send(sock, &msg, sizeof(msg_t), 0) <= 0) die("send OI");

    // Aguardar resposta OI do servidor
    if (recv(sock, &msg, sizeof(msg_t), 0) <= 0) die("recv OI");

    if (ntohs(msg.type) != MSG_OI) {
        fprintf(stderr, "Erro: resposta inesperada do servidor\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor com sucesso!\n");

    // Loop de envio de mensagens
    while (1) {
        char input[MAX_TEXT_LEN + 1];
        int dest_uid;

        printf("Destino (ID ou 0 para todos): ");
        scanf("%d", &dest_uid);

        printf("Mensagem: ");
        getchar(); // limpa o \n do buffer
        fgets((char *)input, sizeof(input), stdin);
        input[strcspn((char *)input, "\n")] = 0; // remover \n

        if (strlen((char *)input) == 0) continue;

        msg.type = htons(MSG_MSG);
        msg.orig_uid = htons(uid);
        msg.dest_uid = htons(dest_uid);
        msg.text_len = htons(strlen((char *)input) + 1);
        strncpy((char *)msg.text, (char *)input, MAX_TEXT_LEN);
        msg.text[MAX_TEXT_LEN] = '\0';

        if (send(sock, &msg, sizeof(msg_t), 0) <= 0) {
            perror("send");
            break;
        }
    }

    // Enviar TCHAU ao sair
    msg.type = htons(MSG_TCHAU);
    msg.orig_uid = htons(uid);
    msg.dest_uid = 0;
    msg.text_len = 0;
    memset(msg.text, 0, sizeof(msg.text));

    send(sock, &msg, sizeof(msg_t), 0);
    close(sock);

    return 0;
}
