#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "utils.h"
#include "user.h"
#include "color.h"

int update_user_name(int sock_fd, int key, char** user_name);
void update_online(int sock_fd, int key, int num_user, int* is_online);
void update_friend(int sock_fd, int key, int num_user, int* is_friend);
void update_unread(int sock_fd, int key, int num_user, int* num_unread);

int print_user(int user_id, int sock_fd, int key, char** user_name);

int username_to_id(int num_user, char**user_name, char* target_name);

int main(int argc, char* argv[]) {
    char address[BUF_SIZE];
    char* host_name;
    char* port;
    int status;
    int sock_fd;
    int key;
    int cookie;
    int num_user;
    int user_id;
    int to_id;
    char msg[BUF_SIZE];
    char username[BUF_SIZE / 2];
    char password[BUF_SIZE / 2];

    char* user_name[MAX_USER];

    struct addrinfo *servinfo;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags    = AI_PASSIVE; 
    
    // Start info
    printf("\
   _____   _   _   _        _____   _   _   ______ \n\
  / ____| | \\ | | | |      |_   _| | \\ | | |  ____|\n\
 | |      |  \\| | | |        | |   |  \\| | | |__   \n\
 | |      | . ` | | |        | |   | . ` | |  __|  \n\
 | |____  | |\\  | | |____   _| |_  | |\\  | | |____ \n\
  \\_____| |_| \\_| |______| |_____| |_| \\_| |______|\n\
                                                   \n\
                                                   \n");
    
    // Connect
    printf("Server address(like www.cn.line:9487): ");
    scanf("%s", address);
    host_name = strtok(address, ":");
    port = strtok(NULL, ":");
    if ((status = getaddrinfo(host_name, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (connect(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen) != 0) {
        fprintf(stderr, "Server unavailable, connect fail\n");
        exit(1);
    }

    // Handshake
    no_crypto_send(sock_fd, "HI", 3, 0);
    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
    if (strcmp(msg, "HI")) {
        fprintf(stderr, "Handshake fail\n");
        exit(1);
    }
    key = 0; // reserve

    // Cookie
    no_crypto_send(sock_fd, "0", 2, 0);
    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response

    // Get auth
    while (1) {
        printf("Register(R)/Login(L): ");
        scanf("%s", msg);
        if (!strcmp(msg, "L")) {
            no_crypto_send(sock_fd, "LGI", 4, 0);
            no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
            printf("\tUsername: ");
            scanf("%s", username);
            printf("\tPassword: ");
            scanf("%s", password);
            sprintf(msg, "%s\t%s", username, password);
            crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
            no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
            if (!strcmp(msg, "LGIDONE")) {
                printf("Login suc!\n");
                break;
            }
            else
                printf("Login fail\n");
        }
        else if (!strcmp(msg, "R")) {
            no_crypto_send(sock_fd, "REG", 4, 0);
            no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
            printf("\tUsername: ");
            scanf("%s", username);
            printf("\tPassword: ");
            scanf("%s", password);
            sprintf(msg, "%s\t%s", username, password);
            crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
            no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
            if (!strcmp(msg, "REGDONE")) {
                printf("Regist suc!\n");
                break;
            }
            else if (!strcmp(msg, "REGDUP")) {
                printf("Username already exist QQ\n");
            }
            else if (!strcmp(msg, "REGWPW")) {
                printf("Password too weak QQ\n");
            }
        }
    }

    // Get cookie, user_name
    no_crypto_send(sock_fd, "REG", 4, 0);
	crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    cookie = atoi(msg);
    for (int i = 0;i < MAX_USER;++i)
        user_name[i] = malloc(sizeof(char) * BUF_SIZE);
    num_user = update_user_name(sock_fd, key, user_name);
    user_id = username_to_id(num_user, user_name, username);
    
    // Welcome info
    printf("\
 ____      ____    __                                   ______               __      _  \n\
|_  _|    |_  _|  [  |                                 |_   _ \\             [  |  _ | | \n\
  \\ \\  /\\  / .---. | | .---.  .--.  _ .--..--. .---.     | |_) | ,--.  .---. | | / ]| | \n\
   \\ \\/  \\/ / /__\\\\| |/ /'`\\/ .'`\\ [ `.-. .-. / /__\\\\    |  __'.`'_\\ :/ /'`\\]| '' < | | \n\
    \\  /\\  /| \\__.,| || \\__.| \\__. || | | | | | \\__.,   _| |__) // | || \\__. | |`\\ \\|_| \n\
     \\/  \\/  '.__.[___'.___.''.__.'[___||__||__'.__.'  |_______/\\'-;__'.___.[__|  \\_(_) \n\
                                                                                        \n");
    // Main loop
    while (1) {
        printf("Who(W)/Message(M)/File(F)/Bye(B): ");

        if (scanf("%s", msg) == EOF)
            break;
        
        else if (!strcmp(msg, "B"))
            break;

        else if (!strcmp(msg, "W")) {
            num_user = print_user(user_id, sock_fd, key, user_name);
            while (1) {
                printf("Add(A)/Delete(D) friend/Continue(C): ");
                scanf("%s", msg);
                if (!strcmp(msg, "A")) {
                    printf("Add who? ");
                    scanf("%s", msg);
                    to_id = username_to_id(num_user, user_name, msg);
                    if (to_id < 0 || to_id > num_user)
                        printf("User not found!\n");
                    else {
                        no_crypto_send(sock_fd, "ADDF", 5, 0);
                        no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
                        sprintf(msg, "%d", to_id);
                        crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
                    }
                }
                else if (!strcmp(msg, "D")) {
                    printf("Delete who? ");
                    scanf("%s", msg);
                    to_id = username_to_id(num_user, user_name, msg);
                    if (to_id < 0 || to_id > num_user)
                        printf("User not found!\n");
                    else {
                        no_crypto_send(sock_fd, "DELF", 5, 0);
                        no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
                        sprintf(msg, "%d", to_id);
                        crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
                    }
                }
                else if (!strcmp(msg, "C"))
                    break;
            }
        }
        else if (!strcmp(msg, "M")) {
        }
        else if (!strcmp(msg, "F")) {
        }
    }

    no_crypto_send(sock_fd, "BYE", 4, 0);
    close(sock_fd);
}

int update_user_name(int sock_fd, int key, char** user_name) {
    int num;
    int ret;
    int count;
    int tmp;
    char msg[BUF_SIZE];

    no_crypto_send(sock_fd, "LIST", 5, 0);
    ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    num = atoi(msg);
    count = 0;

    while (msg[count] != 0)
        ++count;
    ++count;

    for (int i = 0;i < num;++i) {
        if (count == ret) {
            ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
            count = 0;
        }
        tmp = count;
        while (msg[tmp] != 0)
            ++tmp;
        msg[tmp] = 0;
        strcpy(user_name[i], msg + count);
        count = tmp + 1;
    }
    return num;
}


void update_online(int sock_fd, int key, int num_user, int* is_online) {
    int num;
    int ret;
    int count;
    int tmp;
    char msg[BUF_SIZE];

    no_crypto_send(sock_fd, "OLIST", 6, 0);
    ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    num = atoi(msg);
    count = 0;

    while (msg[count] != 0)
        ++count;
    ++count;

    for (int i = 0;i < num;++i) {
        if (count == ret) {
            ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
            count = 0;
        }
        tmp = count;
        while (msg[tmp] != 0)
            ++tmp;
        msg[tmp] = 0;
        is_online[i] = atoi(msg + count);
        count = tmp + 1;
    }
}

void update_friend(int sock_fd, int key, int num_user, int* is_friend) {
    int num;
    int ret;
    int count;
    int tmp;
    char msg[BUF_SIZE];

    no_crypto_send(sock_fd, "FRLIST", 7, 0);
    ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    num = atoi(msg);
    count = 0;

    while (msg[count] != 0)
        ++count;
    ++count;

    for (int i = 0;i < num;++i) {
        if (count == ret) {
            ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
            count = 0;
        }
        tmp = count;
        while (msg[tmp] != 0)
            ++tmp;
        msg[tmp] = 0;
        is_friend[i] = atoi(msg + count);
        count = tmp + 1;
    }
}

void update_unread(int sock_fd, int key, int num_user, int* num_unread) {
    int num;
    int ret;
    int count;
    int tmp;
    char msg[BUF_SIZE];

    no_crypto_send(sock_fd, "UNREAD", 7, 0);
    ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    num = atoi(msg);
    count = 0;

    while (msg[count] != 0)
        ++count;
    ++count;

    for (int i = 0;i < num;++i) {
        if (count == ret) {
            ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
            count = 0;
        }
        tmp = count;
        while (msg[tmp] != 0)
            ++tmp;
        msg[tmp] = 0;
        num_unread[i] = atoi(msg + count);
        count = tmp + 1;
    }
}

#define print_online(is_online) if ((is_online) == ONLINE) {\
                                   set_feature(BG_GREEN);}  \
                                else {                      \
                                   set_feature(BG_RED);}    \
                                printf(" ");                \
                                set_feature(BG_BLACK);

int print_user(int user_id, int sock_fd, int key, char** user_name) {
    int is_online[MAX_USER];
    int is_friend[MAX_USER];
    int num_unread[MAX_USER];
    int num_user;
    num_user = update_user_name(sock_fd, key, user_name);
    update_online(sock_fd, key, num_user, is_online);
    update_friend(sock_fd, key, num_user, is_friend);
    update_unread(sock_fd, key, num_user, num_unread);

    printf("\n=====User List=====\n");
    // not friend, no msg
    for (int i = 0;i < num_user;++i) {
        if (i == user_id || is_friend[i] == 1 || num_unread[i] != 0)
            continue;
        print_online(is_online[i]);
        printf("%s\n", user_name[i]);
    }
    // not friend, has msg
    for (int i = 0;i < num_user;++i) {
        if (i == user_id || is_friend[i] == 1 || num_unread[i] == 0)
            continue;
        print_online(is_online[i]);
        printf("%s(%d)\n", user_name[i], num_unread[i]);
    }
    // is friend, no msg
    for (int i = 0;i < num_user;++i) {
        if (i == user_id || is_friend[i] == 0 || num_unread[i] != 0)
            continue;
        print_online(is_online[i]);
        set_feature(FG_LIGHT_CYAN);
        printf("%s\n", user_name[i]);
        set_feature(FG_LIGHT_WHITE);
    }
    // is friend, has msg
    for (int i = 0;i < num_user;++i) {
        if (i == user_id || is_friend[i] == 0 || num_unread[i] == 0)
            continue;
        print_online(is_online[i]);
        set_feature(FG_LIGHT_CYAN);
        printf("%s(%d)\n", user_name[i], num_unread[i]);
        set_feature(FG_LIGHT_WHITE);
    }
    printf("\n");
    return num_user;
}

int username_to_id(int num_user, char**user_name, char* target_name) {
    int ret = -1;
    for (int i = 0;i < num_user;++i)
        if (!strcmp(user_name[i], target_name)) {
            ret = i;
            break;
        }
    return ret;
}
