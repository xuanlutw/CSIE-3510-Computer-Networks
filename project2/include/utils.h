#ifndef UTILS_H
#define UTILS_H

#include "user.h"

#define BUF_SIZE 1024

// General
#define server_log(...) do{dump_time();printf(__VA_ARGS__);printf("\n");}while(0)
void dump_time();

// Net
ssize_t crypto_recv(int key, int socket, void *buffer, size_t length, int flags);
ssize_t crypto_send(int key, int socket, const void *buffer, size_t length, int flags);

ssize_t no_crypto_recv(int socket, void *buffer, size_t length, int flags);
ssize_t no_crypto_send(int socket, const void *buffer, size_t length, int flags);

#endif
