/*
typedef struct {
    User_info* user_info;
    int port;
} Share_data;

typedef struct {
    Share_data* share_data;
    int sock_fd;
} Thread_data;

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "user.h"
#include "utils.h"

// Data
Share_data* init_share_data(int port) {
    Share_data* share_data = (Share_data*)malloc(sizeof(Share_data));
    share_data->user_info = init_user_info();
    share_data->port = port;
    return share_data;
}
Thread_data* init_thread_data(Share_data* share_data, int sock_fd) {
    Thread_data* thread_data = (Thread_data*)malloc(sizeof(Thread_data));
    thread_data->share_data = share_data;
    thread_data->sock_fd = sock_fd;
    return thread_data;
}

// General
void dump_time() {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] ", buffer);
}

// Net
int init_socket(char* port) {
    int status;
    int sockfd;
    struct addrinfo hints;
    struct addrinfo* serinfo;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags    = AI_PASSIVE; 

    if ((status = getaddrinfo(NULL, port, &hints, &serinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    sockfd = socket(serinfo->ai_family, serinfo->ai_socktype, serinfo->ai_protocol);
    bind(sockfd, serinfo->ai_addr, serinfo->ai_addrlen);
    listen(sockfd, 5);
    return sockfd;
}

int wait_connect(int wel_sock_fd) {
    char str[INET_ADDRSTRLEN];
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(&caddr);
    int fd = accept(wel_sock_fd, (struct sockaddr_in*)&caddr, &clen);
    inet_ntop(AF_INET, &(caddr.sin_addr), str, INET_ADDRSTRLEN);
    server_log("New connection from %s:%d\n", str, caddr.sin_port);
    return fd;
}

// Reserved!
ssize_t crypto_recv(int key, int socket, void *buffer, size_t length, int flags) {
    return recv(socket, buffer, length, flags);
}

ssize_t crypto_send(int key, int socket, const void *buffer, size_t length, int flags) {
    return send(socket, buffer, length, flags);
}
