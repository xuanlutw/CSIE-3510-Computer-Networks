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

int update_user_name();

int main(int argc, char* argv[]) {
    char address[BUF_SIZE];
    char* host_name;
    char* port;
    char* user_name[MAX_USER];
    int status;
    int sock_fd;
    int key;
    int cookie;
    int num_user;
    char msg[BUF_SIZE];
    char username[BUF_SIZE / 2];
    char password[BUF_SIZE / 2];

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
            crypto_send(key, sock_fd, msg, strlen(msg), 0);
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
            crypto_send(key, sock_fd, msg, strlen(msg), 0);
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
    
    for (int i = 0;i < num_user;++i)
        printf("%s\n", user_name[i]);
    // Welcome info
    /*
    printf("\
 ____      ____    __                                   ______               __      _  \n\
|_  _|    |_  _|  [  |                                 |_   _ \\             [  |  _ | | \n\
  \\ \\  /\\  / .---. | | .---.  .--.  _ .--..--. .---.     | |_) | ,--.  .---. | | / ]| | \n\
   \\ \\/  \\/ / /__\\\\| |/ /'`\\/ .'`\\ [ `.-. .-. / /__\\\\    |  __'.`'_\\ :/ /'`\\]| '' < | | \n\
    \\  /\\  /| \\__.,| || \\__.| \\__. || | | | | | \\__.,   _| |__) // | || \\__. | |`\\ \\|_| \n\
     \\/  \\/  '.__.[___'.___.''.__.'[___||__||__'.__.'  |_______/\\'-;__'.___.[__|  \\_(_) \n\
                                                                                        \n");
    while (1) {
        printf("Message(M)/File(F): ");
        scanf("%s", msg);
        if (!strcmp(msg, "M")) {
            printf("List all(A)/online(O)/friend(F): ");
        }
    }
	close(sock_fd);
    */
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
