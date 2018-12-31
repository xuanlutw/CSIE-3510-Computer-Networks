#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <pthread.h>

#include "utils.h"
#include "friend.h"

void read_friend(int user_id, int* fr) {
    char filename[BUF_SIZE];
    FILE* f;
    sprintf(filename, "./data/friend%d", user_id);
    if (access(filename, R_OK|W_OK) != -1) {
        f = fopen(filename, "r");
        for (int i = 0;i < MAX_USER;++i)
            fscanf(f, "%d", fr + i);
        fclose(f);
    }
    else {
        memset(fr, 0, sizeof(int) * MAX_USER);
        back_friend(user_id, fr);
    }
    return;
}

void back_friend(int user_id, int* fr) {
    char filename[BUF_SIZE];
    FILE* f;
    sprintf(filename, "./data/friend%d", user_id);
    f = fopen(filename, "w");
    for (int i = 0;i < MAX_USER;++i)
        fprintf(f, "\t%d", fr[i]);
    fclose(f);
    return;
}

void send_friend(int user_id, int sock_fd, int key) {
    int fr[MAX_USER];
    int counter = 0;
    char msg[BUF_SIZE];
    read_friend(user_id, fr);
    for (int i = 0;i < MAX_USER;++i)
        if (fr[i] != 0)
            ++counter;
    sprintf(msg, "%d", counter);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    for (int i = 0;i < MAX_USER;++i)
        if (fr[i] != 0) {
            sprintf(msg, "%d", i);
            crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
        }
}

int add_friend(int user_id, int sock_fd, int key) {
    int fr[MAX_USER];
    int add_id;
    char msg[BUF_SIZE];

    no_crypto_send(sock_fd, "OKADDF", 7, 0);
    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    add_id = atoi(msg);
    read_friend(user_id, fr);
    fr[add_id] = 1;
    back_friend(user_id, fr);
    return add_id;
}

int del_friend(int user_id, int sock_fd, int key) {
    int fr[MAX_USER];
    int del_id;
    char msg[BUF_SIZE];

    no_crypto_send(sock_fd, "OKDELF", 7, 0);
    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    del_id = atoi(msg);
    read_friend(user_id, fr);
    fr[del_id] = 0;
    back_friend(user_id, fr);
    return del_id;
}
