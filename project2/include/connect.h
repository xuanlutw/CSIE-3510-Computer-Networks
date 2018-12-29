#ifndef CONNECT_H
#define CONNECT_H

#include "init.h"

typedef struct {
    Share_data* share_data;
    int sock_fd;
} Thread_data;

int wait_connect(Share_data *share_data, int wel_sock_fd);
Thread_data* init_thread_data(Share_data* share_data, int sock_fd);
int hand_shake(int sock_fd, int* key);
int check_cookie(int sock_fd, int key);

#endif
