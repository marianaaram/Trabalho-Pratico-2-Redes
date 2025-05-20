#include "protocol.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Converte os campos inteiros da estrutura de mensagem para a ordem de bytes da rede.
 * Necessário para garantir compatibilidade entre diferentes arquiteturas.
 */

void msg_hton(msg_t *msg) {
    msg->type     = htons(msg->type);
    msg->orig_uid = htons(msg->orig_uid);
    msg->dest_uid = htons(msg->dest_uid);
    msg->text_len = htons(msg->text_len);
}


/**
 * Converte os campos inteiros da estrutura de mensagem da ordem da rede para a do host.
 * Deve ser chamada após o recebimento de uma mensagem.
 */
void msg_ntoh(msg_t *msg) {
    msg->type     = ntohs(msg->type);
    msg->orig_uid = ntohs(msg->orig_uid);
    msg->dest_uid = ntohs(msg->dest_uid);
    msg->text_len = ntohs(msg->text_len);
}


/**
 * Envia todos os bytes de um buffer pelo socket.
 * Garante que todo o conteúdo seja transmitido, mesmo que a função send envie apenas parte.
 */
ssize_t send_all(int sockfd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const char *ptr = buffer;
    while (total_sent < length) {
        ssize_t sent = send(sockfd, ptr + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            return sent;
        }
        total_sent += sent;
    }
    return total_sent;
}


/**
 * Recebe todos os bytes esperados de um buffer pelo socket.
 * Garante que todo o conteúdo seja recebido, útil para estruturas fixas como msg_t.
 */
ssize_t recv_all(int sockfd, void *buffer, size_t length) {
    size_t total_received = 0;
    char *ptr = buffer;
    while (total_received < length) {
        ssize_t recvd = recv(sockfd, ptr + total_received, length - total_received, 0);
        if (recvd <= 0) {
            return recvd;
        }
        total_received += recvd;
    }
    return total_received;
}


/**
 * Envia uma estrutura msg_t completa através do socket.
 * Converte os inteiros para ordem de rede antes do envio, e restaura depois.
 */
int send_msg(int sockfd, msg_t *msg) {
    msg_hton(msg);
    ssize_t sent = send_all(sockfd, msg, sizeof(msg_t));
    msg_ntoh(msg); // restaura ordem original
    return (sent == sizeof(msg_t)) ? 0 : -1;
}


/**
 * Recebe uma estrutura msg_t completa através do socket.
 * Converte os inteiros da ordem de rede para a ordem do host após o recebimento.
 */
int recv_msg(int sockfd, msg_t *msg) {
    ssize_t recvd = recv_all(sockfd, msg, sizeof(msg_t));
    if (recvd != sizeof(msg_t)) return -1;
    msg_ntoh(msg);
    return 0;
}
