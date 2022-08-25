#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT 1024
#define BUF 10

void addfd(int epollfd, int fd, bool enable_edge_trigger);

void setnonblock(int fd);

void level_trigger(epoll_event* events, int number, int epollfd, int sock);
void edge_trigger(epoll_event* events, int number, int epollfd, int sock);

int main(int argc, char* argv[] ) {
    if (argc <= 2) {
        printf("usage: %s ip port\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    bool use_edge_trigger = true;

    addfd(epollfd, sock, use_edge_trigger);


    while (1) {
        ret = epoll_wait(epollfd, events, MAX_EVENT, -1);
        if (ret < 0) {
            printf("epoll fail");
            break;
        }

        edge_trigger(events, ret, epollfd, sock);
        // level_trigger(events, ret, epollfd, sock);
    }

    close(sock);

    return 0;
}

void setnonblock(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

/**
 * @brief 
 * 将文件描述符 fd 上的 EPOLLIN 注册到 epollfd 指示的 epoll 内核事件中
 * @param epollfd 
 * @param fd 
 * @param enable_edge_trigger 
 */
void addfd(int epollfd, int fd, bool enable_edge_trigger) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enable_edge_trigger) {
        event.events |= EPOLLET;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblock(fd);
}

void level_trigger(epoll_event* events, int number, int epollfd, int sock) {
    char buf[BUF];
    for (int i = 0; i < number; i++) {
        int sockfd = events[i].data.fd;
        if (sockfd == sock) {  // 服务端套接字
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int connfd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);
            addfd(epollfd, connfd, false);
        } else if (events[i].events & EPOLLIN) {
            printf("event trigger once\n");
            memset(buf, 0, sizeof(buf));

            int ret = recv(sockfd, buf, BUF-1, 0);
            if (ret <= 0) {
                close(sockfd);
                continue;
            }
            printf("get %d bytes of content: %s\n", ret, buf);
        } else {
            printf("something else happened\n");
        }
    }
}

void edge_trigger(epoll_event* events, int number, int epollfd, int sock) {
    char buf[BUF];
    for (int i = 0; i < number; i++) {
        int sockfd = events[i].data.fd;
        if (sockfd == sock) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int connfd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);
            addfd(epollfd, connfd, true);

        } else if (events[i].events & EPOLLIN) {
            printf("event trigger once\n");
            while (1) {
                memset(buf, 0, sizeof(buf));
                int ret = recv(sockfd, buf, BUF-1, 0);
                if (ret < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read later\n");
                        break;
                    }
                    close(sockfd);
                    break;
                } else if (ret == 0) {
                    close(sockfd);
                } else {
                    printf("get %d bytes of content: %s\n", ret, buf);
                }
            }
        } else {
            printf("something else happened\n");
        }
    }
}
