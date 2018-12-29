#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <pthread.h>

#include "cookie.h"

Cookie_info* init_cookie_info() {
    Cookie_info* cookie_info = (Cookie_info*)malloc(sizeof(Cookie_info));
    pthread_mutex_init(&(cookie_info->lock), NULL);
    if (access("./data", R_OK) == -1)
        while (mkdir("./data", S_IRWXU) == -1);
    if (access("./data/cookie_info", R_OK|W_OK) == -1) {
        for (int i = 0;i < MAX_COOKIE; ++i)
            cookie_info->cookie_info_s[i].type = COOKIE_NONE;
        back_cookie_info(cookie_info);
    }
    else {
        FILE* f = fopen("./data/cookie_info", "r");
        for (int i = 0;i < MAX_COOKIE;++i)
            fscanf(f, "%d\t%d\t%d\n", &(cookie_info->cookie_info_s[i].cookie), &(cookie_info->cookie_info_s[i].user), &(cookie_info->cookie_info_s[i].type));
        fclose(f);
    }
    srand(time(NULL)); 
    return cookie_info;
}

void back_cookie_info(Cookie_info* cookie_info) {
    FILE* f = fopen("./data/cookie_info", "w");
    for (int i = 0;i < MAX_COOKIE;++i)
        fprintf(f, "%d\t%d\t%d\n", cookie_info->cookie_info_s[i].cookie, cookie_info->cookie_info_s[i].user, cookie_info->cookie_info_s[i].type);
    fclose(f);
}

int get_cookie_type(Cookie_info* cookie_info, int cookie) {
    int ret = COOKIE_NONE;
    pthread_mutex_lock(&cookie_info->lock);
    for (int i = 0;i < MAX_COOKIE;++i)
        if (cookie_info->cookie_info_s[i].cookie == cookie) {
            ret = cookie_info->cookie_info_s[i].type;
            break;
        }
    pthread_mutex_unlock(&cookie_info->lock);
    return ret;
}

int get_cookie_user(Cookie_info* cookie_info, int cookie) {
    int ret = -1;
    pthread_mutex_lock(&cookie_info->lock);
    for (int i = 0;i < MAX_COOKIE;++i)
        if (cookie_info->cookie_info_s[i].cookie == cookie) {
            ret = cookie_info->cookie_info_s[i].user;
            break;
        }
    pthread_mutex_unlock(&cookie_info->lock);
    return ret;
}

int reg_cookie(Cookie_info* cookie_info, int type, int user) {
    int cookie = rand();
    int fl = 0;
    pthread_mutex_lock(&cookie_info->lock);
    // find blanck
    for (int i = 0;i < MAX_COOKIE;++i)
        if (cookie_info->cookie_info_s[i].type == COOKIE_NONE) {
            cookie_info->cookie_info_s[i].cookie = cookie;
            cookie_info->cookie_info_s[i].user = user;
            cookie_info->cookie_info_s[i].type = type;
            fl = 1;
            break;
        }
    // if no, use first USER
    // it is a bad idea...
    for (int i = 0;i < MAX_COOKIE && !fl;++i)
        if (cookie_info->cookie_info_s[i].type == COOKIE_USER) {
            cookie_info->cookie_info_s[i].cookie = cookie;
            cookie_info->cookie_info_s[i].user = user;
            cookie_info->cookie_info_s[i].type = type;
            fl = 1;
            break;
        }
    back_cookie_info(cookie_info);
    pthread_mutex_unlock(&cookie_info->lock);
    return cookie;
}

void invalid_cookie(Cookie_info* cookie_info, int cookie) {
    pthread_mutex_lock(&cookie_info->lock);
    for (int i = 0;i < MAX_COOKIE;++i)
        if (cookie_info->cookie_info_s[i].cookie == cookie) {
            cookie_info->cookie_info_s[i].type = COOKIE_NONE;
            break;
        }
    back_cookie_info(cookie_info);
    pthread_mutex_unlock(&cookie_info->lock);
}
