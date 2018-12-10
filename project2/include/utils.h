#ifndef UTILS_H
#define UTILS_H

#include "user.h"

// Data
typedef struct {
    User_info* user_info;
    int port;
} Share_data;

typedef struct {
    Share_data* share_data;
    int sock_fd;
} Thread_data;

Share_data* init_share_data(int port);
Thread_data* init_thread_data(Share_data* share_data, int sock_fd);

// General
#define server_log(...) dump_time();printf(__VA_ARGS__)
void dump_time();

// Net
int init_socket(char* port);
int wait_connect(int wel_sock_fd);
ssize_t crypto_recv(int key, int socket, void *buffer, size_t length, int flags);
ssize_t crypto_send(int key, int socket, const void *buffer, size_t length, int flags);


#endif
