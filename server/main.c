#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../../include/protocol.h"

#define MAX_CLIENTS 20
#define SERVER_ID 0
#define PORT 12345

typedef struct {
    int sockfd;
    int uid;
    int is_sender; // 1 = sender, 0 = receiver
} client_t;

client_t clients[MAX_CLIENTS];
int num_clients = 0;
int elapsed_time = 0;
volatile sig_atomic_t send_stats = 0;
time_t server_start;

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void sigalrm_handler(int signo) {
    send_stats = 1;
    alarm(60); // reprograma alarme
}

void add_client(int sockfd, int uid, int is_sender) {
    if (num_clients >= MAX_CLIENTS) return;
    clients[num_clients++] = (client_t){ sockfd, uid, is_sender };
}

void remove_client(int index) {
    close(clients[index].sockfd);
    for (int i = index; i < num_clients - 1; i++)
        clients[i] = clients[i + 1];
    num_clients--;
}

int find_client_by_uid(int uid, int is_sender) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].uid == uid && clients[i].is_sender == is_sender)
            return i;
    }
    return -1;
}

void broadcast_to_receivers(msg_t *msg) {
    for (int i = 0; i < num_clients; i++) {
        if (!clients[i].is_sender) {
            send(clients[i].sockfd, msg, sizeof(msg_t), 0);
        }
    }
}

int main() {
    signal(SIGALRM, sigalrm_handler);
    alarm(60);
    server_start = time(NULL);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) die("socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        die("bind");

    if (listen(listen_fd, 10) < 0)
        die("listen");

    fd_set master_fds, read_fds;
    int fdmax = listen_fd;

    FD_ZERO(&master_fds);
    FD_SET(listen_fd, &master_fds);

    printf("Servidor rodando na porta %d\n", PORT);

    while (1) {
        read_fds = master_fds;
        int activity = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select");
            continue;
        }

        if (send_stats) {
            send_stats = 0;
            msg_t msg;
            msg.type = htons(MSG_MSG);
            msg.orig_uid = htons(SERVER_ID);
            msg.dest_uid = htons(0);
            time_t now = time(NULL);
            int receivers = 0;
            for (int i = 0; i < num_clients; i++)
                if (!clients[i].is_sender) receivers++;

            snprintf((char*)msg.text, sizeof(msg.text),
                     "Servidor ativo há %ld segundos. Exibidores conectados: %d.",
                     now - server_start, receivers);
            msg.text_len = htons(strlen((char*)msg.text) + 1);

            broadcast_to_receivers(&msg);
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listen_fd) {
                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    int newfd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);
                    if (newfd < 0) continue;
                    FD_SET(newfd, &master_fds);
                    if (newfd > fdmax) fdmax = newfd;
                } else {
                    msg_t msg;
                    int bytes = recv(i, &msg, sizeof(msg), 0);
                    if (bytes <= 0) {
                        for (int j = 0; j < num_clients; j++) {
                            if (clients[j].sockfd == i) {
                                printf("Cliente %d desconectado\n", clients[j].uid);
                                remove_client(j);
                                break;
                            }
                        }
                        close(i);
                        FD_CLR(i, &master_fds);
                    } else {
                        int type = ntohs(msg.type);
                        int uid = ntohs(msg.orig_uid);
                        if (type == MSG_OI) {
                            int is_sender = (uid >= 1001);
                            if ((is_sender && find_client_by_uid(uid, 1) != -1) ||
                                (!is_sender && (find_client_by_uid(uid, 0) != -1 ||
                                                find_client_by_uid(uid + 1000, 1) != -1))) {
                                msg_t err;
                                err.type = htons(999); // ERRO fictício
                                send(i, &err, sizeof(msg_t), 0);
                                close(i);
                                FD_CLR(i, &master_fds);
                                continue;
                            }

                            add_client(i, uid, is_sender);
                            send(i, &msg, sizeof(msg_t), 0);
                            printf("Cliente %d conectado (%s)\n", uid, is_sender ? "sender" : "receiver");
                        } else if (type == MSG_MSG) {
                            int sender_index = find_client_by_uid(uid, 1);
                            if (sender_index == -1 || clients[sender_index].sockfd != i)
                                continue;

                            int dest_uid = ntohs(msg.dest_uid);
                            if (dest_uid == 0) {
                                broadcast_to_receivers(&msg);
                            } else {
                                int target = find_client_by_uid(dest_uid, 0);
                                if (target != -1) {
                                    send(clients[target].sockfd, &msg, sizeof(msg_t), 0);
                                }
                            }
                        } else if (type == MSG_TCHAU) {
                            for (int j = 0; j < num_clients; j++) {
                                if (clients[j].sockfd == i) {
                                    printf("Cliente %d saiu (TCHAU)\n", clients[j].uid);
                                    remove_client(j);
                                    break;
                                }
                            }
                            close(i);
                            FD_CLR(i, &master_fds);
                        }
                    }
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}
