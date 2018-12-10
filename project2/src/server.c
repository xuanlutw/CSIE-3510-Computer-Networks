#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

#include "utils.h"
#include "user.h"

#define BUF_SIZE 1024

void* user_handle(void* thread_data);

int main(int argc, char* argv[]) {
    int port;
    int wel_sock_fd;
    int acp_sock_fd;

    // Check argc
    if (argc != 2) {
        fprintf(stderr, "Usage\n%s listen_port\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);

    Share_data* share_data = init_share_data(port);

    // Set socket
    wel_sock_fd = init_socket(argv[1]);
    server_log("Server start on port %d\n", port);

    while (1) {
        acp_sock_fd = wait_connect(wel_sock_fd);
        Thread_data* thread_data = init_thread_data(share_data, acp_sock_fd);
        pthread_t t;
        pthread_create(&t, NULL, user_handle, thread_data);
    }

    //freeaddrinfo(serinfo);
    return 0;
}

void* user_handle(void* thread_data) {
    int key = 0;
    int sock_fd = ((Thread_data*)thread_data)->sock_fd;
    Share_data* share_data = ((Thread_data*)thread_data)->share_data;
    free((Thread_data*)thread_data);

    char recv_msg[BUF_SIZE];

    // Connect and KD
    recv(sock_fd, recv_msg, BUF_SIZE, 0);
    if (strcmp(recv_msg, "HI")) {
        send(sock_fd, "WTF", 4, 0);
        close(sock_fd);
        pthread_exit(NULL);
    }
    else
        send(sock_fd, "HI", 3, 0);
    // do KD...
    
    // Get authorization
    while (1) {
        crypto_recv(key, sock_fd, recv_msg, BUF_SIZE, 0);
        if (!strcmp(recv_msg, "LGI")) {
            crypto_send(key, sock_fd, "OKLGI", 6, 0);
            break;
        }
        else if (!strcmp(recv_msg, "REG")) {
            crypto_send(key, sock_fd, "OKREG", 6, 0);
        }
        else {
            crypto_send(key, sock_fd, "NOAUTH", 7, 0);
        }
    }
    /*
    char ret_msg[16] = "world";
    crypto_recv(0, sock_fd, accept_msg, 16, 0);
    crypto_send(0, sock_fd, ret_msg, 16, 0);
    close(sock_fd);
    printf("Hi\n");
    */
    close(sock_fd);
    pthread_exit(NULL);
}
