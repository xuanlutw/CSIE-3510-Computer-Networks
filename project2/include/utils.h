#ifndef UTILS_H
#define UTILS_H

#include "user.h"

#define BUF_SIZE 1024

// General
#define server_log(LOG_LOCK, ...)   do{\
                                    pthread_mutex_lock(LOG_LOCK);\
                                    dump_time();\
                                    printf(__VA_ARGS__);\
                                    printf("\n");\
                                    pthread_mutex_unlock(LOG_LOCK);\
                                }while(0)
void dump_time(pthread_mutex_t *log_lock);

// Net
ssize_t crypto_recv(int key, int socket, void *buffer, size_t length, int flags);
ssize_t crypto_send(int key, int socket, const void *buffer, size_t length, int flags);

ssize_t no_crypto_recv(int socket, void *buffer, size_t length, int flags);
ssize_t no_crypto_send(int socket, const void *buffer, size_t length, int flags);

#endif
