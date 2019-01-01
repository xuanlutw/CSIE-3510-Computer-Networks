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

// Base 64
static const char base64_alphabet[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '/'};

static const unsigned int base64_suffix_map[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 255,
    255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 253, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
    255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
    7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
    19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255 };


void b64_encode(char* str) {
    char tmp[BUF_SIZE];
    int len;
    int sum;

    strcpy(tmp, str);
    len = strlen(tmp) + 1;
    tmp[len] = 0;
    tmp[len + 1] = 0;
    len = (len + 2) / 3;
    for (int i = 0;i < len;++i) {
        sum = ((int)tmp[3 * i]) + (((int)tmp[3 * i + 1]) << 8) + (((int)tmp[3 * i + 2]) << 16);
        str[4 * i] = base64_alphabet[sum % 64];
        str[4 * i + 1] = base64_alphabet[(sum >> 6)% 64];
        str[4 * i + 2] = base64_alphabet[(sum >> 12) % 64];
        str[4 * i + 3] = base64_alphabet[(sum >> 18) % 64];
    }
    tmp[4 * len] = 0;
}

void b64_decode(char* str) {
    char tmp[BUF_SIZE];
    int len;
    int sum;

    strcpy(tmp, str);
    len = strlen(tmp);
    len = len / 4;
    for (int i = 0;i < len;++i) {
        sum = (base64_suffix_map[(int)tmp[4 * i]]) + (base64_suffix_map[(int)tmp[4 * i + 1]] << 6) + (base64_suffix_map[(int)tmp[4 * i + 2]] << 12) + (base64_suffix_map[(int)tmp[4 * i + 3]] << 18); 
        str[3 * i] = sum % 256;
        str[3 * i + 1] = (sum >> 8) % 256;
        str[3 * i + 2] = (sum >> 16) % 256;
    }
}

// Crypto
// a^b % p
int power_mod(int a, int b) {
    long long ans = 1;
    if (b == 0)
        ans = 1;
    else if (b == 1)
        ans = a;
    else if (!(b % 2)) {
        ans = power_mod(a, b / 2);
        ans = (ans * ans) % DH_P;
    }
    else {
        ans = (power_mod(a, b % 2) * power_mod(a, b - (b % 2))) % DH_P;
    }
    return ans;
}

void encryption(int key, char* buffer, size_t length) {
    for (int i = 0;i < 1; i += 4) {
        buffer[4 * i] += key % 256;
        buffer[4 * i + 1] += (key >> 8) % 256;
        buffer[4 * i + 2] += (key >> 16) % 256;
        buffer[4 * i + 3] += (key >> 24) % 256;
    }
}

void decryption(int key, char* buffer, size_t length) {
    for (int i = 0;i < 1; i += 4) {
        buffer[4 * i] -= key % 256;
        buffer[4 * i + 1] -= (key >> 8) % 256;
        buffer[4 * i + 2] -= (key >> 16) % 256;
        buffer[4 * i + 3] -= (key >> 24) % 256;
    }
}

// Net
ssize_t crypto_recv(int key, int socket, void *buffer, size_t length, int flags) {
    ssize_t ret = recv(socket, buffer, length, flags);
    //decryption(key, buffer, ret);
    /*
    if (*((char*)buffer + (ret - 2)) == '\r') {
        *((char*)buffer + (ret - 2)) = 0;
        ret -= 2;
    }
    else if (*((char*)buffer + (ret - 1)) == '\n') {
        *((char*)buffer + (ret - 1)) = 0;
        ret -= 1;
    }
    */
    return ret;
}

ssize_t crypto_send(int key, int socket, void *buffer, size_t length, int flags) {
    //char buf2[length];
    //for (int i = 0;i < length;++i)
    //    buf2[i] = ((char*)buffer)[i];
    //encryption(key, buf2, length);
    int ret = send(socket, buffer, length, flags);
    //send(socket, "\n", 2, flags);
    return ret;
}

ssize_t no_crypto_recv(int socket, void *buffer, size_t length, int flags) {
    ssize_t ret = recv(socket, buffer, length, flags);
    /*
    if (*((char*)buffer + (ret - 2)) == '\r') {
        *((char*)buffer + (ret - 2)) = 0;
        ret -= 2;
    }
    else if (*((char*)buffer + (ret - 1)) == '\n') {
        *((char*)buffer + (ret - 1)) = 0;
        ret -= 1;
    }
    */
    return ret;
}

ssize_t no_crypto_send(int socket, const void *buffer, size_t length, int flags) {
    int ret = send(socket, buffer, length, flags);
    //send(socket, "\n", 2, flags);
    return ret;
}
