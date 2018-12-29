#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "connect.h"
#include "utils.h"

Thread_data* init_thread_data(Share_data* share_data, int sock_fd) {
    Thread_data* thread_data = (Thread_data*)malloc(sizeof(Thread_data));
    thread_data->share_data = share_data;
    thread_data->sock_fd = sock_fd;
    return thread_data;
}

int wait_connect(int wel_sock_fd) {
    char str[INET_ADDRSTRLEN];
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(&caddr);
    int fd = accept(wel_sock_fd, (struct sockaddr*)&caddr, &clen);
    inet_ntop(AF_INET, &(caddr.sin_addr), str, INET_ADDRSTRLEN);
    server_log("New connection from %s:%d", str, caddr.sin_port);
    return fd;
}

int hand_shake(int sock_fd, int* key) {
    char recv_msg[BUF_SIZE]; 
    no_crypto_recv(sock_fd, recv_msg, BUF_SIZE, 0);
    if (strcmp(recv_msg, "HI"))
        return -1;
    no_crypto_send(sock_fd, "HI", 3, 0);
    return 0;
}
