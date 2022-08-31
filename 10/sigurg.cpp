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

#define BUF 1024

static int connfd;

void sig_urg(int sig);
void add_sig(int sig, void (*sig_handler)(int));

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

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    connfd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);

    if (connfd < 0) {
        perror("accept error");
    } else {
        add_sig(SIGURG, sig_urg);
        fcntl(connfd, F_SETOWN, getpid());

        char buf[BUF];
        while (1) {
            memset(buf, 0, BUF);
            ret = recv(connfd, buf, BUF-1, 0);
            if (ret <= 0) {
                break;
            }
            printf("got %d bytes of normal data %s\n", ret, buf);
        }
        close(connfd);
    }
    close(sock);
    return 0;
}

void sig_urg(int sig) {
    int save_errno = errno;
    char buf[BUF];
    memset(buf, 0, BUF);
    int ret = recv(connfd, buf, BUF-1, MSG_OOB);
    printf("got %d bytes of oob data %s\n", ret, buf);
    errno = save_errno;
}

void add_sig(int sig, void (*sig_handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}
