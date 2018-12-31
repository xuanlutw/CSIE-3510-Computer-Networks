#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "user.h"
#include "utils.h"

// General
void dump_time() {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] ", buffer);
}

// Net
// Reserved!
ssize_t crypto_recv(int key, int socket, void *buffer, size_t length, int flags) {
    ssize_t ret = recv(socket, buffer, length, flags);
    if (*((char*)buffer + (ret - 2)) == '\r') {
        *((char*)buffer + (ret - 2)) = 0;
        ret -= 2;
    }
    else if (*((char*)buffer + (ret - 1)) == '\n') {
        *((char*)buffer + (ret - 1)) = 0;
        ret -= 1;
    }
    return ret;
}

ssize_t crypto_send(int key, int socket, const void *buffer, size_t length, int flags) {
    int ret = send(socket, buffer, length, flags);
    //send(socket, "\n", 2, flags);
    return ret;
}

ssize_t no_crypto_recv(int socket, void *buffer, size_t length, int flags) {
    ssize_t ret = recv(socket, buffer, length, flags);
    if (*((char*)buffer + (ret - 2)) == '\r') {
        *((char*)buffer + (ret - 2)) = 0;
        ret -= 2;
    }
    else if (*((char*)buffer + (ret - 1)) == '\n') {
        *((char*)buffer + (ret - 1)) = 0;
        ret -= 1;
    }
    return ret;
}

ssize_t no_crypto_send(int socket, const void *buffer, size_t length, int flags) {
    int ret = send(socket, buffer, length, flags);
    //send(socket, "\n", 2, flags);
    return ret;
}
