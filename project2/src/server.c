#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

#include "utils.h"
#include "init.h"
#include "connect.h"
#include "user.h"

void* user_handle(void* thread_data);

int main(int argc, char* argv[]) {
    int port;
    int wel_sock_fd;
    int acp_sock_fd;
    pthread_t t;

    // Check argc
    if (argc != 2) {
        fprintf(stderr, "Usage\n%s listen_port\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);

    // Init share data
    Share_data* share_data = init_share_data(port);

    // Init socket
    wel_sock_fd = init_socket(argv[1]);
    server_log("Server start on port %d", port);

    // Main loop, wait connect
    while (1) {
        acp_sock_fd = wait_connect(share_data, wel_sock_fd);
        Thread_data* thread_data = init_thread_data(share_data, acp_sock_fd);
        pthread_create(&t, NULL, user_handle, thread_data);
    }

    //freeaddrinfo(serinfo);
    return 0;
}

void* user_handle(void* thread_data) {
    int key;
    int cookie;
    int user_id;
    int sock_fd = ((Thread_data*)thread_data)->sock_fd;
    Share_data* share_data = ((Thread_data*)thread_data)->share_data;
    free((Thread_data*)thread_data);

    char recv_msg[BUF_SIZE];

    // New thread
    if (hand_shake(sock_fd, &key) == -1) {
        server_log("Socket %d hand shake fail!", sock_fd);
        close(sock_fd);
        pthread_exit(NULL);
    }
    else
        server_log("Socket %d hand shake suc, key = %d", sock_fd, key);
    
    // Crypto thread
    cookie = check_cookie(sock_fd, key);
    user_id = get_cookie_user(share_data->cookie_info, cookie);
    switch (get_cookie_type(share_data->cookie_info, cookie)) {
        case COOKIE_USER:
            no_crypto_send(sock_fd, "AUTHOK", 7, 0);
            server_log("Socket %d check cookie = %d, type user, user_id = %d", sock_fd, cookie, user_id);
            if (get_auth(share_data, user_id, sock_fd, pthread_self()) < 0) {
                close(sock_fd);
                pthread_exit(NULL);
            }
            break;
        case COOKIE_FILE:
            // reserve
            server_log("Socket %d check cookie = %d, type file, user_id = %d", sock_fd, cookie, user_id);
            break;
        case COOKIE_NONE:
            no_crypto_send(sock_fd, "AUTHFAIL", 9, 0);
            server_log("Socket %d check cookie = %d, type none", sock_fd, cookie);
            break;
    }

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
