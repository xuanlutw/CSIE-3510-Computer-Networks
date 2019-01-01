#ifndef CONNECT_H
#define CONNECT_H

#include <pthread.h>
#include "init.h"

typedef struct {
    Share_data* share_data;
    int sock_fd;
    char ip[INET_ADDRSTRLEN];
} Thread_data;

int wait_connect(Share_data *share_data, int wel_sock_fd, char* str);
Thread_data* init_thread_data(Share_data* share_data, int sock_fd, char* ip);
int hand_shake(int sock_fd, int* key);
int check_cookie(int sock_fd, int key);

int attach_auth(Share_data* share_data, int user_id, int sock_fd, pthread_t handle);
int get_auth(Share_data* share_data, int* user_id, int sock_fd, pthread_t handle, int key);
void transfer_file(Share_data* share_data, int sock_fd, int key, int cookie);

#endif
