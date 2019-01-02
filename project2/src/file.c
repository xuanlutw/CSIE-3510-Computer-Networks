#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>

#include "utils.h"
#include "file.h"

File_info* init_file_info() {
    File_info* file_info = (File_info*)malloc(sizeof(File_info));
    for (int i = 0;i < MAX_USER;++i)
        pthread_mutex_init(&(file_info->lock[i]), NULL);
    return file_info;
}

int read_filelist(int user_id, File_list* file_list) {
    int ret = 0;
    char filename[BUF_SIZE];
    FILE* f;
    sprintf(filename, "./data/file%d", user_id);
    if (access(filename, R_OK|W_OK) != -1) {
        f = fopen(filename, "r");
        while (fscanf(f, "%d\t%d\t%s", &(file_list[ret].from), &(file_list[ret].status), file_list[ret].filename) != EOF)
            ++ret;
        fclose(f);
    }
    else
        back_filelist(user_id, 0, file_list);
    
    return ret;
}

void back_filelist(int user_id, int num_file, File_list* file_list) {
    char filename[BUF_SIZE];
    FILE* f;
    sprintf(filename, "./data/file%d", user_id);
    f = fopen(filename, "w");
    for (int i = 0;i < num_file;++i)
        fprintf(f, "%d\t%d\t%s\n", file_list[i].from, file_list[i].status, file_list[i].filename);
    fclose(f);
    return;
}

void send_filelist(File_info* file_info, int user_id, int sock_fd, int key) {
    File_list file_list[MAX_FILE];
    char msg[BUF_SIZE];
    int num_file;

    pthread_mutex_lock(file_info->lock + user_id);
    num_file = read_filelist(user_id, file_list);
    sprintf(msg, "%d", num_file);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    for (int i = 0;i < num_file;++i) {
        sprintf(msg, "%d\t%d\t%s", file_list[i].from, file_list[i].status, file_list[i].filename);
        crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    }
    pthread_mutex_unlock(file_info->lock + user_id);
}

void valid_file(File_info* file_info, int user_id, int file_id) {
    File_list file_list[MAX_FILE];
    int num_file;

    pthread_mutex_lock(file_info->lock + user_id);
    num_file = read_filelist(user_id, file_list);
    file_list[file_id].status = 1;
    back_filelist(user_id, num_file, file_list);

    pthread_mutex_unlock(file_info->lock + user_id);
}

int send_file(File_info* file_info, Cookie_info* cookie_info, int user_id, int sock_fd, int key) {
    char msg[BUF_SIZE];
    char* saveptr;
    int to_id;
    int num_file;
    int cookie;
    File_list file_list[MAX_FILE];
    
    no_crypto_send(sock_fd, "OKFSEND", 8, 0);
    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    to_id = atoi(strtok_r(msg, "\t", &saveptr));
    
    pthread_mutex_lock(file_info->lock + to_id);

    num_file = read_filelist(to_id, file_list);
    
    // Special case, when user_id = 0, file_id = 0, direction fail !!
    // To prevent this case, give a never ready data
    if (user_id == 0 && num_file == 0) {
        ++num_file;
        file_list[0].from = 0;
        file_list[0].status = 0; // Never ready !
        strcpy(file_list[0].filename, "system_reserve");
    }

    // updata file list
    file_list[num_file].from = user_id;
    file_list[num_file].status = 0;
    strcpy(file_list[num_file].filename, strtok_r(NULL, "\t", &saveptr));
    ++num_file;
    back_filelist(to_id, num_file, file_list);

    pthread_mutex_unlock(file_info->lock + to_id);

    // give cookie
    cookie = reg_file_cookie(cookie_info, to_id, num_file - 1, S_FILE);
    sprintf(msg, "%d", cookie);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    return to_id;
}

int read_file(File_info* file_info, Cookie_info* cookie_info, int user_id, int sock_fd, int key) {
    char msg[BUF_SIZE];
    int num_file;
    int file_id;
    int cookie;
    File_list file_list[MAX_FILE];
    
    no_crypto_send(sock_fd, "OKFREAD", 8, 0);
    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    file_id = atoi(msg);
    
    pthread_mutex_lock(file_info->lock + user_id);

    // updata file list
    num_file = read_filelist(user_id, file_list);
    pthread_mutex_unlock(file_info->lock + user_id);

    // give cookie
    if (file_id >= num_file || file_list[file_id].status == 0) {
        crypto_send(key, sock_fd, "0", 2, 0);
        return -1;
    }
    cookie = reg_file_cookie(cookie_info, user_id, file_id, R_FILE);
    sprintf(msg, "%d", cookie);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    return file_id;
}
