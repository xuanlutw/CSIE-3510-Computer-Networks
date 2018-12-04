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

typedef struct {
    int socket_fd;
    int num;
    char* host_name;
    char* port;
    struct timeval t_stamp;
    struct addrinfo* host_info;
    int state;  // 0: try connect, 1 try send
} Connect_info;

int do_connect(Connect_info *to_connect);
int fd_get_connect(Connect_info* connect_info, int num_connect, int fd);
    
int main(int argc, char* argv[]) {
    int status;
    int sockfd;
    int isock;

    int num = 0;
    int timeout = 1000;
    int arg_err_fl = 0;
    int num_connect = argc - 1;

    // Check argc
    for (int i = 1;i < argc;++i) {
        if (!strcmp(argv[i], "-n")) {
            if (i + 1 < argc) {
                num = atoi(argv[i + 1]);
                (argv[i])[0] = 0;
                (argv[i + 1])[0] = 0;
                num_connect -= 2;
            }
            else 
                arg_err_fl = 1;
        }
        if (!strcmp(argv[i], "-t")) {
            if (i + 1 < argc) {
                timeout = atoi(argv[i + 1]);
                (argv[i])[0] = 0;
                (argv[i + 1])[0] = 0;
                num_connect -= 2;
            }
            else 
                arg_err_fl = 1;
        }
    }
    if (arg_err_fl || argc < 2) {
        fprintf(stderr, "Usage\n%s [-n number] [-t timeout] host_1:port_1 host_2:port_2 ...\n", argv[0]);
        exit(1);
    }

    // Init connect info
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags    = AI_PASSIVE; 

    Connect_info connect_info[num_connect];
    for (int i = 1, j = 0;i < argc;++i) {
        if ((argv[i])[0] == 0) continue;
        connect_info[j].num       = num - (num == 0);   // 0 -> -1, inf loop
        connect_info[j].host_name = strtok(argv[i], ":");
        connect_info[j].port      = strtok(NULL, ":");
        connect_info[j].state     = 0;
        if ((status = getaddrinfo(connect_info[j].host_name, connect_info[j].port, &hints, &(connect_info[j].host_info))) != 0) {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            exit(1);
        }
        ++j;
    }
    
    // Connect all
    int max_fd;
    fd_set fd_connect, fd_c_back;
    fd_set fd_read, fd_r_back;
    FD_ZERO(&fd_connect);
    FD_ZERO(&fd_read);
    
    for (int i = 0;i < num_connect;++i)
        if (do_connect(connect_info + i) == 0) {
            FD_SET(connect_info[i].socket_fd, &fd_connect);
            if (max_fd < connect_info[i].socket_fd)
                max_fd = connect_info[i].socket_fd;
        }

    // Select loop
    int remain = num_connect;
    struct timeval tv;
    
    while (remain) {
        int connect_id, fd_id, flag;
        struct timeval t_now;
        char ip_str[INET_ADDRSTRLEN];

        memcpy(&fd_c_back, &fd_connect, sizeof(fd_set));
        memcpy(&fd_r_back, &fd_read, sizeof(fd_set));
        tv.tv_sec = timeout / 1000;  
        tv.tv_usec = (timeout % 1000) * 1000;

        int ret = select(max_fd + 1, &fd_read, &fd_connect, NULL, &tv);
        if (ret < 1) {
            // time out
            for (int i = 0;i <= max_fd;++i) {
                if (FD_ISSET(i, &fd_c_back)) {
                    fd_id = i;
                    flag = 0;
                    connect_id = fd_get_connect(connect_info, num_connect, i);
                    break;
                }
                if (FD_ISSET(i, &fd_r_back)) {
                    fd_id = i;
                    flag = 1;
                    connect_id = fd_get_connect(connect_info, num_connect, i);
                    break;
                }
            }
            close(connect_info[connect_id].socket_fd);
            
            //printf("%d %d\n", fd_id, connect_id);
            inet_ntop(AF_INET, &(((struct sockaddr_in*)(connect_info[connect_id].host_info->ai_addr))->sin_addr), ip_str, INET_ADDRSTRLEN);
            printf("timeout when connect to %s\n", ip_str);
        }
        else {
            // check
            for (int i = 0;i <= max_fd;++i) {
                if (FD_ISSET(i, &fd_connect)) {
                    fd_id = i;
                    flag = 0;
                    connect_id = fd_get_connect(connect_info, num_connect, i);
                    break;
                }
                if (FD_ISSET(i, &fd_read)) {
                    fd_id = i;
                    flag = 1;
                    connect_id = fd_get_connect(connect_info, num_connect, i);
                    break;
                }
            }

            if (connect_info[connect_id].state == 0) {
                char send_msg[] = "hello"; 
                gettimeofday(&(connect_info[connect_id].t_stamp), 0);
                send(connect_info[connect_id].socket_fd, send_msg, 16, 0);
            }
            else {
                close(connect_info[connect_id].socket_fd);

                //printf("%d %d %d>\n", max_fd, fd_id, connect_id);
                inet_ntop(AF_INET, &(((struct sockaddr_in*)(connect_info[connect_id].host_info->ai_addr))->sin_addr), ip_str, INET_ADDRSTRLEN);
                gettimeofday(&t_now, 0);
                printf("recv from %s, RTT = %d msec\n", ip_str, (t_now.tv_sec - connect_info[connect_id].t_stamp.tv_sec) * 1000 + (t_now.tv_usec - connect_info[connect_id].t_stamp.tv_usec) / 1000);
            }
        }
        // do next
        memcpy(&fd_connect, &fd_c_back, sizeof(fd_set));
        memcpy(&fd_read, &fd_r_back, sizeof(fd_set));
        if (flag == 0) FD_CLR(fd_id, &fd_connect);
        else FD_CLR(fd_id, &fd_read);


        if (ret == 0 || flag == 1) {
            connect_info[connect_id].state = 0;
            if (do_connect(connect_info + connect_id) == 1) {
                --remain;
            }
            else {
                FD_SET(connect_info[connect_id].socket_fd, &fd_connect);
                if (max_fd < connect_info[connect_id].socket_fd)
                    max_fd = connect_info[connect_id].socket_fd;
            }
        }
        else {
            connect_info[connect_id].state = 1;
            FD_SET(connect_info[connect_id].socket_fd, &fd_read);
        }
    }
    freeaddrinfo(connect_info[0].host_info);
    return 0;
}

int do_connect(Connect_info *to_connect) {
    // return 1, no check fd
    // return 0, check fd
    
    if (to_connect->num == 0) {
        to_connect->socket_fd = -1;
        return 1;
    }
    to_connect->socket_fd = socket(to_connect->host_info->ai_family, to_connect->host_info->ai_socktype, to_connect->host_info->ai_protocol);
    fcntl(to_connect->socket_fd, F_SETFL, O_NONBLOCK);
    if (to_connect->num != -1)
        --(to_connect->num);
    gettimeofday(&(to_connect->t_stamp), 0); 
    int ret = connect(to_connect->socket_fd, to_connect->host_info->ai_addr, to_connect->host_info->ai_addrlen);
    if (ret == 0) {
        struct timeval t_now;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in*)(to_connect->host_info->ai_addr))->sin_addr), ip_str, INET_ADDRSTRLEN);
        gettimeofday(&t_now, 0);
        printf("recv from %s, RTT = %d msec\n", ip_str, (t_now.tv_sec - to_connect->t_stamp.tv_sec) * 1000 + (t_now.tv_usec - to_connect->t_stamp.tv_usec) / 1000);
        return do_connect(to_connect);
    }
    return 0;
}

int fd_get_connect(Connect_info* connect_info, int num_connect, int fd) {
    for (int i = 0;i < num_connect;++i)
        if (connect_info[i].socket_fd == fd)
            return i;
    return -1;
}
