#ifndef COOKIE_H
#define COOKIE_H

#include <pthread.h>

#define MAX_COOKIE 1024
#define COOKIE_NONE 0
#define COOKIE_USER 1
#define COOKIE_FILE 2

#include "utils.h"

typedef struct {
    int cookie;
    int user;
    int type;
} Cookie_info_s;

typedef struct {
    Cookie_info_s cookie_info_s[MAX_COOKIE];
    pthread_mutex_t lock;
} Cookie_info;

Cookie_info* init_cookie_info();
void back_cookie_info(Cookie_info* cookie_info);

int get_cookie_type(Cookie_info* cookie_info, int cookie);
int get_cookie_user(Cookie_info* cookie_info, int cookie);
int reg_cookie(Cookie_info* cookie_info, int type, int user_id);
void invalid_cookie(Cookie_info* cookie_info, int cookie);

#define S_FILE 0
#define R_FILE 1
int reg_file_cookie(Cookie_info* cookie_info, int to_id, int file_id, int direction);
#define to_id_of_cookie(x)      ((x) / MAX_FILE)
#define file_id_of_cookie(x)    ((x) % MAX_FILE)
#define direction_of_cookie(x)  ((x) >= 0? S_FILE: R_FILE)

int send_cookie(Cookie_info* cookie_info, int cookie, int user_id, int sock_fd, int key);

#endif
