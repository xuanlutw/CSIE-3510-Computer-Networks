#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
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
        unread = memset(unread, 0, sizeof(int) * MAX_USER);
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
    int counter = 0;
    char msg[BUF_SIZE];
    pthread_mutex_lock(msg_info->lock + user_id);
    read_unread(user_id, unread);
    for (int i = 0;i < MAX_USER;++i)
        if (unread[i] != 0)
            ++counter;
    sprintf(msg, "%d", counter);
    crypto_send(key, sock_fd, msg, strlen(msg), 0);
    for (int i = 0;i < MAX_USER;++i)
        if (unread[i] != 0) {
            sprintf(msg, "%d\t%d", i, unread[i]);
            crypto_send(key, sock_fd, msg, strlen(msg), 0);
        }
    pthread_mutex_unlock(msg_info->lock + user_id);
}

void send_msg(Msg_info* msg_info, int user_id, int sock_fd, int key) {
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
}

/*
void back_user_info(User_info* user_info) {
    FILE* f = fopen("./data/user_info", "w");
    for (int i = 0;i < user_info->user_num;++i)
        fprintf(f, "%s\t%s\n", user_info->user_info_s[i].username, user_info->user_info_s[i].password);
    fclose(f);
}

int get_user_status(User_info* user_info, int user_id) {
    int ret = OFFLINE;
    pthread_mutex_lock(&user_info->lock);
    if (user_id < user_info->user_num)
        ret = user_info->user_info_s[user_id].status;
    pthread_mutex_unlock(&user_info->lock);
    return ret;
}

int get_user_num(User_info* user_info) {
    pthread_mutex_lock(&user_info->lock);
    int ret = user_info->user_num;
    pthread_mutex_unlock(&user_info->lock);
    return ret;
}

int get_online_user_num(User_info* user_info) {
    pthread_mutex_lock(&user_info->lock);
    int ret = user_info->online_user_num;
    pthread_mutex_unlock(&user_info->lock);
    return ret;
}

int get_user_id(User_info* user_info, char* username) {
    int ret = -1;
    pthread_mutex_lock(&user_info->lock);
    for (int i = 0;i < user_info->user_num;++i)
        if (!strcmp(username, user_info->user_info_s[i].username)) {
            ret = i;
            break;
        }
    pthread_mutex_unlock(&user_info->lock);
    return ret;
}

int valid_user(User_info* user_info, char* username, char* password) {
    pthread_mutex_lock(&user_info->lock);
    int ret = -1;
    for (int i = 0;i < user_info->user_num;++i)
        if (!strcmp(username, user_info->user_info_s[i].username) && !strcmp(password, user_info->user_info_s[i].password)) {
            ret = i;
            break;
        }
    pthread_mutex_unlock(&user_info->lock);
    return ret;
}

int user_attach(User_info* user_info, int user_id, int sock_fd, pthread_t handle) {
    int ret = 0;
    pthread_mutex_lock(&user_info->lock);
    if (user_info->user_info_s[user_id].status == ONLINE)
        ret = -1;
    user_info->user_info_s[user_id].status = ONLINE;
    user_info->user_info_s[user_id].handle = handle;
    user_info->user_info_s[user_id].sock_fd = sock_fd;
    pthread_mutex_unlock(&user_info->lock);
    return ret;
}

void user_detach(User_info* user_info, int user_id) {
    pthread_mutex_lock(&user_info->lock);
    user_info->user_info_s[user_id].status = OFFLINE;
    pthread_mutex_unlock(&user_info->lock);
    return;
}


int user_regist(User_info* user_info, char* username, char* password) {
    int user_id = get_user_id(user_info, username);
    pthread_mutex_lock(&user_info->lock);
    if (user_id != -1)
        user_id = D_USERNAME;
    else if (strlen(password) > 16 || strlen(password) < 8)
        user_id = W_PASSWORD;
    else {
        user_id = user_info->user_num;
        strcpy(user_info->user_info_s[user_id].username, username);
        strcpy(user_info->user_info_s[user_id].password, password);
        user_info->user_info_s[user_id].status = OFFLINE;
        ++(user_info->user_num);
    }
    back_user_info(user_info);
    pthread_mutex_unlock(&user_info->lock);
    return user_id;
}
*/
