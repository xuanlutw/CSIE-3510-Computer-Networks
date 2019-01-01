#ifndef FILE_H
#define FILE_H

#include <pthread.h>
#include "user.h"
#include "utils.h"
#include "cookie.h"

#define MAX_FILE 1024

typedef struct {
    pthread_mutex_t lock[MAX_USER];
} File_info;

typedef struct {
    int from;
    int status;
    char filename[BUF_SIZE / 2];
} File_list;

File_info* init_file_info();

// both are not thread safe
int read_filelist(int user_id, File_list* file_list);
void back_filelist(int user_id, int num_file, File_list* file_list);

void send_filelist(File_info* file_info, int user_id, int sock_fd, int key);
void valid_file(File_info* file_info, int user_id, int file_id);

int send_file(File_info* file_info, Cookie_info* cookie_info, int user_id, int sock_fd, int key);
int read_file(File_info* file_info, Cookie_info* cookie_info, int user_id, int sock_fd, int key);

#endif
