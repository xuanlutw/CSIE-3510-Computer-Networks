#ifndef MSG_H
#define MSG_H

#include <pthread.h>
#include "user.h"
#include "utils.h"

typedef struct {
    pthread_mutex_t lock[MAX_USER];
} Msg_info;

Msg_info* init_msg_info();

// both are not thread safe
void read_unread(int user_id, int* unread);
void back_unread(int user_id, int* unread);

void send_unread(Msg_info* msg_info, int user_id, int sock_fd, int key);
void send_msg(Msg_info* msg_info, int user_id, int sock_fd, int key);
/*
int get_user_status(User_info* user_info, int user_id);

int get_user_num(User_info* user_info);
int get_online_user_num(User_info* user_info);
int get_user_id(User_info* user_info, char* username);

int valid_user(User_info* user_info, char* username, char* password);

int user_attach(User_info* user_info, int user_id, int sock_fd, pthread_t handle);
void user_detach(User_info* user_info, int user_id);

int user_regist(User_info* user_info, char* username, char* password);
*/

#endif
