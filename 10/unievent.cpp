#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <signal.h>

#define MAX_EVENT 1024
static int pipefd[2];

int setnonblock(int fd);
void addfd(int epollfd, int fd);
void sig_handler(int sig);
void add_sig(int sig);

int main(int argc, char* argv[]) {
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

    addfd(epollfd, sock);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblock(pipefd[1]);
    addfd(epollfd, pipefd[0]);

    add_sig(SIGHUP);
    add_sig(SIGCHLD);
    add_sig(SIGTERM);
    add_sig(SIGINT);

    bool stop_server = false;
    while (!stop_server) {
        int number = epoll_wait(epollfd, events, MAX_EVENT, -1);
        if (number < 0 && errno != EAGAIN) {
            perror("epoll wait failed");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == sock) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);
                int connfd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);
                addfd(epollfd, connfd);
            } else if (sockfd == pipefd[0] && events[i].events & EPOLLIN) {
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1 || ret == 0) {
                    continue;
                } else {
                    for (int j = 0; j < ret; j++) {
                        printf("I caugh the signal %d\n", signals[i]);
                        switch (signals[i]) {
                            case SIGCHLD:
                            case SIGHUP:
                                continue;
                            case SIGTERM:
                            case SIGINT:
                                stop_server = true;
                        }
                    }
                }
            }
        }
    }

    close(sock);
    close(pipefd[1]);
    close(pipefd[0]);
    return 0;
}

int setnonblock(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblock(fd);
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], reinterpret_cast<char*>(&msg), 1, 0);
    errno = save_errno;
}

void add_sig(int sig) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}
