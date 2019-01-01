#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>

#include "utils.h"
#include "msg.h"

Msg_info* init_msg_info() {
    Msg_info* msg_info = (Msg_info*)malloc(sizeof(Msg_info));
    for (int i = 0;i < MAX_USER;++i)
        pthread_mutex_init (&(msg_info->lock[i]), NULL);
    return msg_info;
}

void read_unread(int user_id, int* unread) {
    char filename[BUF_SIZE];
    FILE* f;
    sprintf(filename, "./data/unread%d", user_id);
    if (access(filename, R_OK|W_OK) != -1) {
        f = fopen(filename, "r");
        for (int i = 0;i < MAX_USER;++i)
            fscanf(f, "%d", unread + i);
        fclose(f);
    }
    else {
        memset(unread, 0, sizeof(int) * MAX_USER);
        back_unread(user_id, unread);
    }
    return;
}

void back_unread(int user_id, int* unread) {
    char filename[BUF_SIZE];
    FILE* f;
    sprintf(filename, "./data/unread%d", user_id);
    f = fopen(filename, "w");
    for (int i = 0;i < MAX_USER;++i)
        fprintf(f, "\t%d", unread[i]);
    fclose(f);
    return;
}

void send_unread(Msg_info* msg_info, int user_id, int sock_fd, int key) {
    int unread[MAX_USER];
    char msg[BUF_SIZE];
    pthread_mutex_lock(msg_info->lock + user_id);
    read_unread(user_id, unread);
    sprintf(msg, "%d", MAX_USER);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    for (int i = 0;i < MAX_USER;++i) {
        sprintf(msg, "%d", unread[i]);
        crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    }
    pthread_mutex_unlock(msg_info->lock + user_id);
}

int send_msg(Msg_info* msg_info, int user_id, int sock_fd, int key) {
    char filename[BUF_SIZE];
    char msg[BUF_SIZE];
    char* saveptr;
    int to_id;
    int unread[MAX_USER];
    
    no_crypto_send(sock_fd, "OKSEND", 7, 0);
    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    to_id = atoi(strtok_r(msg, "\t", &saveptr));
    
    // prevent dead lock
    if (to_id > user_id) {
        pthread_mutex_lock(msg_info->lock + user_id);
        pthread_mutex_lock(msg_info->lock + to_id);
        sprintf(filename, "./data/msg%d-%d", user_id, to_id);
    }
    else {
        pthread_mutex_lock(msg_info->lock + to_id);
        pthread_mutex_lock(msg_info->lock + user_id);
        sprintf(filename, "./data/msg%d-%d", to_id, user_id);
    }

    // write msg
    FILE* f = fopen(filename, "a");
    fprintf(f, "%d\t%s\n", user_id, strtok_r(NULL, "\t", &saveptr));
    fclose(f);
    
    // update unread
    read_unread(to_id, unread);
    ++unread[user_id];
    back_unread(to_id, unread);

    pthread_mutex_unlock(msg_info->lock + user_id);
    pthread_mutex_unlock(msg_info->lock + to_id);
    return to_id;
}

int read_msg(Msg_info* msg_info, int user_id, int sock_fd, int key) {
    char filename[BUF_SIZE];
    char msg[BUF_SIZE];
    char out_msg[BUF_SIZE + 100];
    int to_id;
    int src_id;
    int counter = 0;
    int unread[MAX_USER];
    FILE* f;
    
    no_crypto_send(sock_fd, "OKREAD", 7, 0);
    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    to_id = atoi(msg);
    
    // prevent dead lock
    if (to_id > user_id) {
        pthread_mutex_lock(msg_info->lock + user_id);
        pthread_mutex_lock(msg_info->lock + to_id);
        sprintf(filename, "./data/msg%d-%d", user_id, to_id);
    }
    else {
        pthread_mutex_lock(msg_info->lock + to_id);
        pthread_mutex_lock(msg_info->lock + user_id);
        sprintf(filename, "./data/msg%d-%d", to_id, user_id);
    }

    // creat file
    int fd2 = open (filename, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    close(fd2);

    // send counter
    f = fopen(filename, "r");
    while (fscanf(f, "%d\t%s", &src_id, msg) != EOF)
        ++counter;
    fclose(f);
    sprintf(msg, "%d", counter);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    
    // send and update unread
    read_unread(user_id, unread);
    sprintf(msg, "%d", unread[to_id]);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    unread[to_id] = 0;
    back_unread(user_id, unread);

    // send msg
    f = fopen(filename, "r");
    while (fscanf(f, "%d\t%s", &src_id, msg) != EOF) {
        sprintf(out_msg, "%d\t%s", src_id, msg);
        crypto_send(key, sock_fd, out_msg, strlen(out_msg) + 1, 0);
    }
    fclose(f);

    pthread_mutex_unlock(msg_info->lock + user_id);
    pthread_mutex_unlock(msg_info->lock + to_id);
    return to_id;
}

int send_anonymous(Msg_info* msg_info, int user_id, int sock_fd, int key) {
    char filename[BUF_SIZE];
    char msg[BUF_SIZE];
    char* saveptr;
    int to_id;
    
    no_crypto_send(sock_fd, "OKASEND", 8, 0);
    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    to_id = atoi(strtok_r(msg, "\t", &saveptr));
    
    pthread_mutex_lock(msg_info->lock + to_id);
    sprintf(filename, "./data/anon%d", to_id);

    // write msg
    FILE* f = fopen(filename, "a");
    fprintf(f, "%s\n", strtok_r(NULL, "\t", &saveptr));
    fclose(f);
    
    pthread_mutex_unlock(msg_info->lock + to_id);
    return to_id;
}

int read_anonymous(Msg_info* msg_info, int user_id, int sock_fd, int key) {
    char filename[BUF_SIZE];
    char msg[BUF_SIZE];
    char out_msg[BUF_SIZE + 100];
    int counter = 0;
    FILE* f;
    
    pthread_mutex_lock(msg_info->lock + user_id);
    sprintf(filename, "./data/anon%d", user_id);

    // creat file
    int fd2 = open(filename, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    close(fd2);

    // send counter
    f = fopen(filename, "r");
    while (fscanf(f, "%s\n", msg) != EOF)
        ++counter;
    fclose(f);
    sprintf(msg, "%d", counter);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    
    // send msg
    f = fopen(filename, "r");
    while (fscanf(f, "%s", msg) != EOF) {
        sprintf(out_msg, "%s", msg);
        crypto_send(key, sock_fd, out_msg, strlen(out_msg) + 1, 0);
    }
    fclose(f);

    pthread_mutex_unlock(msg_info->lock + user_id);
    return user_id;
}
