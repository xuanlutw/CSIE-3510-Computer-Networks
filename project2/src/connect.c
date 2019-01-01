#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "connect.h"
#include "utils.h"

Thread_data* init_thread_data(Share_data* share_data, int sock_fd, char* ip) {
    Thread_data* thread_data = (Thread_data*)malloc(sizeof(Thread_data));
    thread_data->share_data = share_data;
    thread_data->sock_fd = sock_fd;
    strcpy(thread_data->ip, ip);
    return thread_data;
}

int wait_connect(Share_data* share_data, int wel_sock_fd, char* str) {
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(&caddr);
    int fd = accept(wel_sock_fd, (struct sockaddr*)&caddr, &clen);
    inet_ntop(AF_INET, &(caddr.sin_addr), str, INET_ADDRSTRLEN);
    server_log("New connection from %s:%d", str, caddr.sin_port);
    return fd;
}

int hand_shake(int sock_fd, int* key) {
    char msg[BUF_SIZE]; 
    int a = rand();
    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
    if (strcmp(msg, "HI"))
        return -1;
    no_crypto_send(sock_fd, "HI", 3, 0);
    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
    *key = power_mod(atoi(msg), a);
    sprintf(msg, "%d", power_mod(DH_G, a));
    no_crypto_send(sock_fd, msg, strlen(msg) + 1, 0);
    return 0;
}

int check_cookie(int sock_fd, int key) {
    char recv_msg[BUF_SIZE]; 
    crypto_recv(key, sock_fd, recv_msg, BUF_SIZE, 0);
    return atoi(recv_msg);
}

int attach_auth(Share_data* share_data, int user_id, int sock_fd, pthread_t handle) {
    if (user_attach(share_data->user_info, user_id, sock_fd, pthread_self()) < 0) {
        server_log("Socket %d user dup! user_id = %d", sock_fd, user_id);
        no_crypto_send(sock_fd, "DUP", 4, 0);
        return -1;
    }
    return 0;
}

int get_auth(Share_data* share_data, int* user_id, int sock_fd, pthread_t handle, int key) {
    char recv_msg[BUF_SIZE]; 
    char* username;
    char* password;
    char* saveptr;
    int count = 5;
    while (count--) {
        no_crypto_recv(sock_fd, recv_msg, BUF_SIZE, 0);
        if (!strcmp(recv_msg, "REG")) {
            no_crypto_send(sock_fd, "OKREG", 6, 0);
            crypto_recv(key, sock_fd, recv_msg, BUF_SIZE, 0);
            username = strtok_r(recv_msg, "\t", &saveptr);
            password = strtok_r(NULL, "\t", &saveptr);
            *user_id = user_regist(share_data->user_info, username, password);
            if (*user_id == W_PASSWORD)
                no_crypto_send(sock_fd, "REGWPW", 7, 0);
            else if (*user_id == D_USERNAME)
                no_crypto_send(sock_fd, "REGDUP", 7, 0);
            else {
                no_crypto_send(sock_fd, "REGDONE", 8, 0);
                server_log("Socket %d register suc! user_id = %d, username = %s, password = %s", sock_fd, *user_id, username, password);
                return attach_auth(share_data, *user_id, sock_fd, handle);
            }
        }
        else if (!strcmp(recv_msg, "LGI")) {
            no_crypto_send(sock_fd, "OKLGI", 6, 0);
            crypto_recv(key, sock_fd, recv_msg, BUF_SIZE, 0);
            username = strtok_r(recv_msg, "\t", &saveptr);
            password = strtok_r(NULL, "\t", &saveptr);
            if ((*user_id = valid_user(share_data->user_info, username, password)) < 0)
                no_crypto_send(sock_fd, "LGIREJ", 7, 0);
            else {
                no_crypto_send(sock_fd, "LGIDONE", 8, 0);
                server_log("Socket %d login suc!", sock_fd);
                return attach_auth(share_data, *user_id, sock_fd, handle);
            }
        }
    }
    no_crypto_send(sock_fd, "OVERFAIL", 9, 0);
    server_log("Socket %d get auth fail!", sock_fd);
    return -1;
}

void transfer_file(Share_data* share_data, int sock_fd, int key, int cookie) {
    char filename[BUF_SIZE];
    char msg[BUF_SIZE];
    int fd;
    int ret;
    int cookie_info;

    // Determine cookie
    cookie_info = get_cookie_user(share_data->cookie_info, cookie);
    sprintf(filename, "./data/file%d-%d", to_id_of_cookie(cookie_info), file_id_of_cookie(cookie_info));
    fd = open(filename, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);

    if (direction_of_cookie(cookie_info) == S_FILE) {
        while (1) {
            if ((ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0)) <= 0)
                break;
            write(fd, msg, ret);
        }
        valid_file(share_data->file_info, to_id_of_cookie(cookie_info), file_id_of_cookie(cookie_info));
    }
    else if (direction_of_cookie(cookie_info) == R_FILE) {
        while (1) {
            if ((ret = read(fd, msg, BUF_SIZE)) <= 0)
                break;
            crypto_send(key, sock_fd, msg, ret, 0);
        }
    }
    close(fd);

    return;
}
