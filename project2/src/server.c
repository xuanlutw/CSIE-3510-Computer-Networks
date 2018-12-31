#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

#include "utils.h"
#include "init.h"
#include "connect.h"
#include "user.h"
#include "msg.h"
#include "friend.h"

void* user_handle(void* thread_data);

int main(int argc, char* argv[]) {
    int port;
    int wel_sock_fd;
    int acp_sock_fd;
    pthread_t t;

    // Disable SIGPIPE before any net connection
    signal(SIGPIPE, SIG_IGN);

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
    int ret;
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
            if (attach_auth(share_data, user_id, sock_fd, pthread_self()) < 0) {
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
            if (get_auth(share_data, &user_id, sock_fd, pthread_self(), key) < 0) {
                close(sock_fd);
                pthread_exit(NULL);
            }
            break;
    }

    // Clean cookie
    invalid_cookie(share_data->cookie_info, cookie);
    cookie = 0;
    server_log("User %d init!", user_id);

    // Main loop
    while (1) {
        ret = no_crypto_recv(sock_fd, recv_msg, BUF_SIZE, 0);
        // client crash
        if (ret <= 0) {
            server_log("User %d crash @@", user_id);
            break;
        }
        // msg
        if (!strcmp(recv_msg, "LIST")) {
            send_user_list(share_data->user_info, sock_fd, key);
            server_log("User %d LIST", user_id);
        }
        else if (!strcmp(recv_msg, "UNREAD")) {
            send_unread(share_data->msg_info, user_id, sock_fd, key);
            server_log("User %d UNREAD", user_id);
        }
        else if (!strcmp(recv_msg, "SEND")) {
            ret = send_msg(share_data->msg_info, user_id, sock_fd, key);
            server_log("User %d SEND to %d", user_id, ret);
        }
        else if (!strcmp(recv_msg, "READ")) {
            ret = read_msg(share_data->msg_info, user_id, sock_fd, key);
            server_log("User %d read from %d", user_id, ret);
        }
        // friend
        else if (!strcmp(recv_msg, "FRLIST")) {
            send_friend(user_id, sock_fd, key);
            server_log("User %d FRLIST", user_id);
        }
        else if (!strcmp(recv_msg, "ADDF")) {
            ret = add_friend(user_id, sock_fd, key);
            server_log("User %d ADDF, lucky guy = %d", user_id, ret);
        }
        else if (!strcmp(recv_msg, "DELF")) {
            ret = del_friend(user_id, sock_fd, key);
            server_log("User %d DELF, poor guy = %d", user_id, ret);
        }
        // cookie
        else if (!strcmp(recv_msg, "ASKCK")) {
            cookie = send_cookie(share_data->cookie_info, cookie, user_id, sock_fd, key);
            server_log("User %d ASKCK, cookie = %d", user_id, cookie);
        }
        // bye
        else if (!strcmp(recv_msg, "BYE")) {
            server_log("User %d BYE", user_id);
            break;
        }
        else {
            no_crypto_send(sock_fd, "WTF", 4, 0);
            server_log("User %d murmur...", user_id);
        }
    }
    user_detach(share_data->user_info, user_id);
    invalid_cookie(share_data->cookie_info, cookie);
    no_crypto_send(sock_fd, "BYE", 4, 0);
    close(sock_fd);
    pthread_exit(NULL);
}
