#ifndef UTILS_H
#define UTILS_H

#include "user.h"

#define BUF_SIZE 1024

// General
#define server_log(...)     do{\
                                pthread_mutex_lock(&(share_data->log_lock));\
                                dump_time();\
                                printf(__VA_ARGS__);\
                                printf("\n");\
                                pthread_mutex_unlock(&(share_data->log_lock));\
                            }while(0)
void dump_time();

// Base 64
// Those functions WILL change the rough string and assume NO memory issue
void b64_encode(char* str);
void b64_decode(char* str);

// Crypto
#define DH_P 2147483647
#define DH_G 5
int power_mod(int a, int b);
void encryption(int key, char* buffer, size_t length);
void decryption(int key, char* buffer, size_t length);

// Net
ssize_t crypto_recv(int key, int socket, void *buffer, size_t length, int flags);
ssize_t crypto_send(int key, int socket, void *buffer, size_t length, int flags);

ssize_t no_crypto_recv(int socket, void *buffer, size_t length, int flags);
ssize_t no_crypto_send(int socket, const void *buffer, size_t length, int flags);

#endif
