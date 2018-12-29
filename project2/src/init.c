#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "init.h"

Share_data* init_share_data(int port) {
    Share_data* share_data = (Share_data*)malloc(sizeof(Share_data));
    share_data->user_info = init_user_info();
    share_data->port = port;
    return share_data;
}

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
