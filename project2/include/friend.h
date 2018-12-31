#ifndef FRIEND_H
#define FRIEND_H

#include <pthread.h>
#include "user.h"
#include "utils.h"

// friend is not share data, NO lock

void read_friend(int user_id, int* fr);
void back_friend(int user_id, int* fr);

void send_friend(int user_id, int sock_fd, int key);
int add_friend(int user_id, int sock_fd, int key);
int del_friend(int user_id, int sock_fd, int key);

#endif
