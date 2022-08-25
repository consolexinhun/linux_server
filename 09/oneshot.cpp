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

struct fds {
    int epollfd;
    int sockfd;
};

void setnonblock(int fd);
void addfd(int epollfd, int fd, bool oneshot);

void* worker(void* arg);

void reset_oneshot(int epollfd, int sockfd);

int main(int argc, char** argv) {
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

    /* 监听套接字不能注册 EPOLLONESHOT 事件的，否则只能处理一个客户端连接 */
    addfd(epollfd, sock, false);

    while (1) {
        ret = epoll_wait(epollfd, events, MAX_EVENT, -1);
        if (ret < 0) {
            printf("epoll fail\n");
            break;
        }

        for (int i = 0; i < ret; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == sock) {  // 服务端套接字
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);
                int connfd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);

                addfd(epollfd, connfd, true);
            } else if (events[i].events & EPOLLIN) {
                pthread_t th;
                fds fds_worker;
                fds_worker.epollfd = epollfd;
                fds_worker.sockfd = sockfd;
                pthread_create(&th, NULL, worker, reinterpret_cast<void*>(&fds_worker));
            } else {
                puts("some thing else");
            }
        }
    }

    close(sock);

    return 0;
}

void setnonblock(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}

void addfd(int epollfd, int fd, bool oneshot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblock(fd);
}

void* worker(void* arg) {
    int sockfd = (reinterpret_cast<fds*>(arg))->sockfd;
    int epollfd = (reinterpret_cast<fds*>(arg))->epollfd;

    printf("start new thread to receive data on fd : %d\n", sockfd);
    char buf[BUF];
    memset(buf, 0, sizeof(buf));

    while (1) {
        int ret = recv(sockfd, buf, BUF-1, 0);
        if (ret == 0) {
            close(sockfd);
            printf("foreiner closed the connection\n");
            break;
        } else if (ret < 0) {
            if (errno == EAGAIN) {
                reset_oneshot(epollfd, sockfd);
                printf("read later\n");
                break;
            }
        } else {
            printf("get content: %s\n", buf);
            sleep(5);
        }
    }
    printf("end thread reveiveing data on fd: %d\n", sockfd);
    return NULL;
}

void reset_oneshot(int epollfd, int sockfd) {
    epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
}
