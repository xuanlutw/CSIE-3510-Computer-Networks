#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "utils.h"
#include "user.h"
#include "file.h"
#include "color.h"

static void pipe_handle(int nSigno);

typedef struct {
    char filename[BUF_SIZE / 2];
    char* host_name;
    char* port;
    int cookie;
    int direction;
} File_thread_data;

#define MAX_RECON   120
jmp_buf buf;

int update_user_name(int sock_fd, int key, char** user_name);
void update_online(int sock_fd, int key, int num_user, int* is_online);
void update_friend(int sock_fd, int key, int num_user, int* is_friend);
void update_unread(int sock_fd, int key, int num_user, int* num_unread);

int print_user(int user_id, int sock_fd, int key, char** user_name);

int username_to_id(int num_user, char**user_name, char* target_name);

void print_msg(int user_id, int to_id, int sock_fd, int key, char** user_name);
void print_anonymous(int user_id, int sock_fd, int key, char** user_name);
void handle_msg(int user_id, int sock_fd, int key, int num_user, char** user_name);
void handle_anonymous(int user_id, int sock_fd, int key, int num_user, char** user_name);

int update_file(int sock_fd, int key, File_list* file_list);
int print_file(int user_id, int sock_fd, int key, char** user_name, int num_file, File_list* file_list);
int filename_to_id(int num_file, File_list* file_list, char* target_name);

void* file_thread_handle(void* thread_data);

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
    int l;
    int ret;
    char msg[BUF_SIZE];
    char msg2[BUF_SIZE / 2];
    char username[BUF_SIZE / 2];
    char password[BUF_SIZE / 2];

    pthread_t t;
    File_thread_data file_thread_data;

    char* user_name[MAX_USER];

    File_list file_list[MAX_FILE];
    int num_file;

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
    int b = rand();
    sprintf(msg, "%d", power_mod(DH_G, b));
    no_crypto_send(sock_fd, msg, strlen(msg) + 1, 0);
    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
    key = power_mod(atoi(msg), b);

    // Cookie
    crypto_send(key, sock_fd, "0", 2, 0);
    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response

    while (1) {
        printf("Register(r)/Login(l): ");
        scanf("%s", msg);
        if (!strcmp(msg, "l")) {
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
                break;
            }
            else
                printf("Login fail\n");
        }
        else if (!strcmp(msg, "r")) {
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

    // Reconnection
    if (setjmp(buf)) {
        printf("Connection lose QQ\n");
        for (int i = 0;i < MAX_RECON;++i) {
            printf("Trying %d / %d...\n", i, MAX_RECON);
            status = getaddrinfo(host_name, port, &hints, &servinfo);
            sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
            if (!connect(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen))
                break;
            sleep(1);
        }

        // Handshake, again...
        no_crypto_send(sock_fd, "HI", 3, 0);
        no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
        if (strcmp(msg, "HI")) {
            fprintf(stderr, "Handshake fail\n");
            exit(1);
        }
        key = 0; // reserve
        
        sprintf(msg, "%d", cookie);
        crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
        no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
    }

    // Get auth
    // Get cookie, user_name
    no_crypto_send(sock_fd, "ASKCK", 6, 0);
	crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    cookie = atoi(msg);
    for (int i = 0;i < MAX_USER;++i)
        user_name[i] = malloc(sizeof(char) * BUF_SIZE);
    num_user = update_user_name(sock_fd, key, user_name);
    user_id = username_to_id(num_user, user_name, username);
   
    // Start handle SIGPIPE
    signal(SIGPIPE , &pipe_handle);

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
        printf("Who(w)/Message(m)/File(f)/Anonymous(a)/Bye(b): ");

        // For test
        if (scanf("%s", msg) == EOF)
            break;
        
        //Bye
        else if (!strcmp(msg, "b"))
            break;

        // Who
        else if (!strcmp(msg, "w")) {
            num_user = print_user(user_id, sock_fd, key, user_name);
            while (1) {
                printf("Add(a)/Delete(d) friend/Message(m)/Continue(c): ");
                scanf("%s", msg);
                if (!strcmp(msg, "a")) {
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
                else if (!strcmp(msg, "d")) {
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
                else if (!strcmp(msg, "m")) {
                    handle_msg(user_id, sock_fd, key, num_user,  user_name);
                    break;
                }
                else
                    break;
            }
        }

        // Message
        else if (!strcmp(msg, "m")) {
            handle_msg(user_id, sock_fd, key, num_user,  user_name);
        }

        // Anonymous
        else if (!strcmp(msg, "a")) {
            handle_anonymous(user_id, sock_fd, key, num_user,  user_name);
        }

        // File
        else if (!strcmp(msg, "f")) {
            printf("Send(s)/Read(r): ");
            scanf("%s", msg);
            file_thread_data.port = port;
            file_thread_data.host_name = host_name;
            if (!strcmp(msg, "s")) {
                do {
                    printf("To who? ");
                    scanf("%s", msg);
                    to_id = username_to_id(num_user, user_name, msg);
                    if (to_id < 0 || to_id > num_user) {
                        printf("User not found!\n");
                        break;
                    }
                    printf("Choose file? ");
                    scanf("%s", msg2);
                    if (access(msg2, R_OK) == -1) {
                        printf("File not found!\n");
                        break;
                    }
                    no_crypto_send(sock_fd, "FSEND", 6, 0);
                    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
                    sprintf(msg, "%d\t%s", to_id, msg2);
                    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
                    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
                    // Creat new thread!
                    file_thread_data.cookie = atoi(msg);
                    file_thread_data.direction = S_FILE;
                    strcpy(file_thread_data.filename, msg2);
                    pthread_create(&t, NULL, file_thread_handle, &file_thread_data);
                } while (0);
            }
            else if (!strcmp(msg, "r")) {
                do {
                    num_file = update_file(sock_fd, key, file_list);
                    ret = print_file(user_id, sock_fd, key, user_name, num_file, file_list);
                    if (!ret)
                        break;
                    getc(stdin); //'\n'
                    printf("Download which file? ");
                    fgets(msg2, BUF_SIZE / 2, stdin);
                    l = strlen(msg2);
                    msg2[l - 1] = 0;
                    if (!strcmp(msg2, ""))
                        break;
                    to_id = filename_to_id(num_file, file_list, msg2);
                    if (to_id < 0) {
                        printf("File not found!\n");
                        break;
                    }
                    no_crypto_send(sock_fd, "FREAD", 6, 0);
                    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
                    sprintf(msg, "%d", to_id);
                    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
                    crypto_recv(key, sock_fd, msg, BUF_SIZE, 0); // Cookie
                    // Creat new thread!
                    file_thread_data.cookie = atoi(msg);
                    file_thread_data.direction = R_FILE;
                    strcpy(file_thread_data.filename, msg2);
                    pthread_create(&t, NULL, file_thread_handle, &file_thread_data);
                } while (0);
            }
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

#define MSG_SHOW 20
void print_msg(int user_id, int to_id, int sock_fd, int key, char** user_name) {
    int num;
    int un_num;
    int count;
    int ret;
    int tmp;
    char msg[BUF_SIZE];

    int id;
    char bmsg[BUF_SIZE];

    no_crypto_send(sock_fd, "READ", 5, 0);
    no_crypto_recv(sock_fd, msg, BUF_SIZE, 0);
    sprintf(msg, "%d", to_id);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    num = atoi(msg);   
    count = 0;
    while (msg[count] != 0) {
        ++count;
    }
    ++count;

    if (count == ret) {
        ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
        count = 0;
    }
    un_num = atoi(msg + count);   
    while (msg[count] != 0) {
        ++count;
    }
    ++count;

    printf("\n=====Message=====\n");
    for (int i = 0;i < num;++i) {
        if (count == ret) {
            ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
            count = 0;
        }
        tmp = count;
        while (msg[tmp] != 0)
            ++tmp;
        sscanf(msg + count, "%d\t%s", &id, bmsg);
        count = tmp + 1;
        if (num - i <= un_num) {
            b64_decode(bmsg);
            set_feature(FG_LIGHT_BLUE);
            printf("%s\t: %s\n", user_name[id], bmsg);
            set_feature(FG_LIGHT_WHITE);
        }
        else if (num - i <= MSG_SHOW) {
            b64_decode(bmsg);
            printf("%s\t: %s\n", user_name[id], bmsg);
        }
    }
    printf("\n");
}

void print_anonymous(int user_id, int sock_fd, int key, char** user_name) {
    int num;
    int count;
    int ret;
    int tmp;
    char msg[BUF_SIZE];

    char bmsg[BUF_SIZE];

    no_crypto_send(sock_fd, "AREAD", 6, 0);
    ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
    num = atoi(msg);   
    count = 0;
    while (msg[count] != 0) {
        ++count;
    }
    ++count;
    printf("%d\n", num);

    printf("\n=====Anonymous Box=====\n");
    for (int i = 0;i < num;++i) {
        if (count == ret) {
            ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0);
            count = 0;
        }
        tmp = count;
        while (msg[tmp] != 0)
            ++tmp;
        sscanf(msg + count, "%s", bmsg);
        count = tmp + 1;
        if (num - i <= MSG_SHOW) {
            b64_decode(bmsg);
            printf("\t: %s\n", bmsg);
        }
    }
    printf("\n");
}

void handle_msg(int user_id, int sock_fd, int key, int num_user, char** user_name) {
    char msg[BUF_SIZE];
    char msg2[BUF_SIZE / 2];
    int to_id;
    int l;

    printf("With who? ");
    scanf("%s", msg);
    to_id = username_to_id(num_user, user_name, msg);
    if (to_id < 0 || to_id > num_user) {
        printf("User not found!\n");
        return;
    }

    getc(stdin); //'\n'
    while(1) {
        print_msg(user_id, to_id, sock_fd, key, user_name);
        printf("%s\t: ", user_name[user_id]);
        fgets(msg2, BUF_SIZE / 2, stdin);
        l = strlen(msg2);
        msg2[l - 1] = 0;
        if (!strcmp(msg2, ""))
            break;
        b64_encode(msg2);
        no_crypto_send(sock_fd, "SEND", 5, 0);
        no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
        sprintf(msg, "%d\t%s", to_id, msg2);
        crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
    }
}

void handle_anonymous(int user_id, int sock_fd, int key, int num_user, char** user_name) {
    char msg[BUF_SIZE];
    char msg2[BUF_SIZE / 2];
    int to_id;
    int l;
    printf("Send(s)/Read(r): ");
    scanf("%s", msg);
    if (!strcmp(msg, "s")) {
        do {
            printf("To who? ");
            scanf("%s", msg);
            to_id = username_to_id(num_user, user_name, msg);
            if (to_id < 0 || to_id > num_user) {
                printf("User not found!\n");
                break;
            }
            printf("\t : ");
            getc(stdin);
            fgets(msg2, BUF_SIZE / 2, stdin);
            l = strlen(msg2);
            msg2[l - 1] = 0;
            if (!strcmp(msg2, ""))
                break;
            b64_encode(msg2);
            no_crypto_send(sock_fd, "ASEND", 6, 0);
            no_crypto_recv(sock_fd, msg, BUF_SIZE, 0); // Skip response
            sprintf(msg, "%d\t%s", to_id, msg2);
            crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0);
        } while (0);
    }
    else if (!strcmp(msg, "r")) {
        print_anonymous(user_id, sock_fd, key, user_name);
    }
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

int update_file(int sock_fd, int key, File_list* file_list) {
    int num;
    int ret;
    int count;
    int tmp;
    char msg[BUF_SIZE];

    no_crypto_send(sock_fd, "FLIST", 6, 0);
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
        sscanf(msg + count, "%d\t%d\t%s", &(file_list[i].from), &(file_list[i].status), file_list[i].filename);
        count = tmp + 1;
    }
    return num;
}

int print_file(int user_id, int sock_fd, int key, char** user_name, int num_file, File_list* file_list) {
    int ret = 0;
    printf("\n=======FILE=======\n");
    for (int i = 0;i < num_file;++i) {
        if (file_list[i].status == 0)
            continue;
        printf("%s\t: %s\n", user_name[file_list[i].from], file_list[i].filename);
        ++ret;
    }
    if (!ret)
        printf("(no file)");
    printf("\n");
    return ret;
}

int filename_to_id(int num_file, File_list* file_list, char* target_name) {
    int ret = -1;
    for (int i = 0;i < num_file;++i)
        if (file_list[i].status && !strcmp(file_list[i].filename, target_name)) {
            ret = i;
            break;
        }
    return ret;
}

void* file_thread_handle(void* thread_data) {
    char filename[BUF_SIZE / 2];
    char msg[BUF_SIZE];
    int fd;
    int ret;
    int cookie;
    int sock_fd;
    int status;
    int key;
    int direction;

    char* host_name;
    char* port;
    struct addrinfo *servinfo;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags    = AI_PASSIVE; 
    
    // copy data
    strcpy(filename, ((File_thread_data*)thread_data)->filename);
    port = ((File_thread_data*)thread_data)->port;
    host_name = ((File_thread_data*)thread_data)->host_name;
    cookie = ((File_thread_data*)thread_data)->cookie;
    direction = ((File_thread_data*)thread_data)->direction;

    // Socket
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
    sprintf(msg, "%d", cookie);
    crypto_send(key, sock_fd, msg, strlen(msg) + 1, 0); // NO response

    // Determine cookie

    if (direction == R_FILE) {
        if (access("./download", R_OK) == -1)
            while (mkdir("./download", S_IRWXU) == -1);
        sprintf(msg, "./download/%s", filename);
        fd = open(msg, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        while (1) {
            if ((ret = crypto_recv(key, sock_fd, msg, BUF_SIZE, 0)) <= 0)
                break;
            write(fd, msg, ret);
        }
    }
    else if (direction == S_FILE) {
        fd = open(filename, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        while (1) {
            if ((ret = read(fd, msg, BUF_SIZE)) <= 0)
                break;
            crypto_send(key, sock_fd, msg, ret, 0);
        }
    }
    close(fd);
    close(sock_fd);
    pthread_exit(NULL);
}

static void pipe_handle(int nSigno) {
    signal(nSigno, pipe_handle);
    longjmp(buf, 1);
}
