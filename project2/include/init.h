#ifndef INIT_H
#define INIT_H

#include <pthread.h>
#include "user.h"

typedef struct {
    User_info* user_info;
    int port;
    pthread_mutex_t log_lock;
} Share_data;

Share_data* init_share_data(int port);
int init_socket(char* port);

#endif
