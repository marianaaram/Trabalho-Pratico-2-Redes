#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../include/protocol.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Cria o socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Define endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Erro no endereço IP");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Conecta ao servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar ao servidor");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor. Aguardando mensagens...\n");

    // Loop para receber mensagens
    msg_t msg;
    while (1) {
        int ret = recv_msg(sockfd, &msg);
        if (ret < 0) {
            perror("Erro ao receber mensagem");
            break;
        }

        // Exibe conteúdo da mensagem
        msg.text[msg.text_len] = '\0'; // Garante que o texto é string válida
        printf("Mensagem recebida (tipo %d) de %d: %s\n", msg.type, msg.orig_uid, msg.text);

        if (msg.type == MSG_TCHAU) {
            printf("Recebido MSG_TCHAU. Encerrando conexão.\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}
