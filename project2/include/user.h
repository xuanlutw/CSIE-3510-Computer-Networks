#ifndef USER_H
#define USER_H

#include <pthread.h>

#define MAX_USER 1024
#define MAX_USERNAME 16
#define MAX_PASSWORD 16

#define ONLINE  1
#define OFFLINE 0

#define W_PASSWORD -1
#define D_USERNAME -2

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
    pthread_mutex_t lock;
} User_info;

User_info* init_user_info();
void back_user_info(User_info* user_info);

int get_user_status(User_info* user_info, int user_id);

int get_user_num(User_info* user_info);
int get_online_user_num(User_info* user_info);
int get_user_id(User_info* user_info, char* username);

int valid_user(User_info* user_info, char* username, char* password);

int user_attach(User_info* user_info, int user_id, int sock_fd, pthread_t handle);
void user_detach(User_info* user_info, int user_id);

int user_regist(User_info* user_info, char* username, char* password);

void send_user_list(User_info* user_info, int sock_fd, int key);

#endif
