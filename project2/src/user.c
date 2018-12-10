/*
#define MAX_USER 1024
#define MAX_USERNAME 16
#define MAX_PASSWORD 16

#define ONLINE  1
#define OFFLINE 0

#define REG_FAIL -1
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char status;
    int sock_fd;
    pthread_t handle;
} User_info_s;

typedef struct {
    int user_num;
    int online_user_num;
    User_info_s user_info_s[MAX_USER];
} User_info;
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <pthread.h>

#include "user.h"

User_info* init_user_info() {
    User_info* user_info = (User_info*)malloc(sizeof(User_info));
    pthread_mutex_init (&(user_info->lock), NULL);
    if (access("./data", R_OK) == -1)
        while (mkdir("./data", S_IRWXU) == -1);
    if (access("./data/user_info", R_OK|W_OK) == -1) {
        user_info->user_num = 0;
        user_info->online_user_num = 0;
        for (int i = 0;i < MAX_USER; ++i) {
            user_info->user_info_s[i].username[0] = 0;
            user_info->user_info_s[i].password[0] = 0;
            user_info->user_info_s[i].status = OFFLINE;
        }
    }
    else {
        FILE* f = fopen("./data/user_info", "r");
        user_info->online_user_num = 0;
        int i = 0;
        while (fscanf(f, "%s\t%s", user_info->user_info_s[i].username, user_info->user_info_s[i].password) != EOF) {
            user_info->user_info_s[i].status = OFFLINE;
            ++i;
        }
        user_info->user_num = i;
        fclose(f);
    }
    return user_info;
}

void back_user_info(User_info* user_info) {
    pthread_mutex_lock(&user_info->lock);
    FILE* f = fopen("./data/user_info", "w");
    for (int i = 0;i < user_info->user_num;++i)
        fprintf(f, "%s\t%s\n", user_info->user_info_s[i].username, user_info->user_info_s[i].password);
    fclose(f);
    pthread_mutex_unlock(&user_info->lock);
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

void user_attach(User_info* user_info, int user_id, int sock_fd, pthread_t handle) {
    pthread_mutex_lock(&user_info->lock);
    if (user_info->user_info_s[user_id].status == OFFLINE) {
        pthread_cancel(user_info->user_info_s[user_id].handle);
        send(user_info->user_info_s[user_id].sock_fd, "MUL_LGI", 8, 0);
        close(user_info->user_info_s[user_id].sock_fd);
    }
    user_info->user_info_s[user_id].status = OFFLINE;
    user_info->user_info_s[user_id].handle = handle;
    user_info->user_info_s[user_id].sock_fd = sock_fd;
    pthread_mutex_unlock(&user_info->lock);
    return;
}
void user_detach(User_info* user_info, int user_id) {
    pthread_mutex_lock(&user_info->lock);
    user_info->user_info_s[user_id].status = OFFLINE;
    pthread_mutex_unlock(&user_info->lock);
    return;
}


int user_regist(User_info* user_info, char* username, char* password) {
    pthread_mutex_lock(&user_info->lock);
    pthread_mutex_unlock(&user_info->lock);
    return 0;
}

