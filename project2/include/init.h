#ifndef INIT_H
#define INIT_H

#include "user.h"

typedef struct {
    User_info* user_info;
    int port;
} Share_data;

Share_data* init_share_data(int port);
int init_socket(char* port);

#endif
