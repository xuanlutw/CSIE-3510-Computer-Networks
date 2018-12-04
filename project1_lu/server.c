#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    int status;
    int port;
    int sockfd;
    int isock;
    char str[INET_ADDRSTRLEN];
    struct addrinfo hints;
    struct addrinfo* serinfo;
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(&caddr);

    // Check argc
    if (argc != 2) {
        fprintf(stderr, "Usage\n%s listen_port\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);

    // Set socket
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags    = AI_PASSIVE; 

    if ((status = getaddrinfo(NULL, argv[1], &hints, &serinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    sockfd = socket(serinfo->ai_family, serinfo->ai_socktype, serinfo->ai_protocol);
    bind(sockfd, serinfo->ai_addr, serinfo->ai_addrlen);
    listen(sockfd, 5);

    while (1) {
        char accept_msg[16];
        char ret_msg[16] = "world";
        isock = accept(sockfd, (struct sockaddr_in*)&caddr, &clen);
        inet_ntop(AF_INET, &(caddr.sin_addr), str, INET_ADDRSTRLEN);
        printf("recv from %s:%d\n", str, caddr.sin_port);
        recv(isock, accept_msg, 16, 0);
#ifdef SLOW
        sleep(1);
#endif
        send(isock, ret_msg, 16, 0);
        close(isock);
    }

    freeaddrinfo(serinfo);
    return 0;
}
