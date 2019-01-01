#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "init.h"

Share_data* init_share_data(int port) {
    Share_data* share_data = (Share_data*)malloc(sizeof(Share_data));
    share_data->user_info = init_user_info();
    share_data->cookie_info = init_cookie_info();
    share_data->msg_info = init_msg_info();
    share_data->file_info = init_file_info();
    share_data->port = port;
    pthread_mutex_init(&(share_data->log_lock), NULL);
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
    
    if ((sockfd = socket(serinfo->ai_family, serinfo->ai_socktype, serinfo->ai_protocol)) < 0) {
        fprintf(stderr, "start socket error: %s\n", strerror(errno));
        exit(1);
    }

    if ((bind(sockfd, serinfo->ai_addr, serinfo->ai_addrlen)) < 0) {
        fprintf(stderr, "bind port %s error: %s\n", port, strerror(errno));
        exit(1);
    }

    listen(sockfd, 5);
    return sockfd;
}
