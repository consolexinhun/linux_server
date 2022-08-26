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

#define BUF 1024

int setnonblock(int fd);

int nonblock_connect(const char* ip, int port, int time);

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip port\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sock = nonblock_connect(ip, port, 10);
    if (sock < 0) {
        puts("nonblock error");
        return 1;
    }
    shutdown(sock, SHUT_WR);
    sleep(200);
    printf("send data out\n");
    send(sock, "abc", 3, 0);
    return 0;
}

int setnonblock(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int nonblock_connect(const char* ip, int port, int time) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int fdopt = setnonblock(sock);
    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == 0) {
        printf("connect with server\n");
        fcntl(sock, F_SETFL, fdopt);
        return sock;
    } else if (errno != EINPROGRESS) {
        printf("nonblock connect not support \n");
        return -1;
    }
    /* 只有当连接还没建立并且 errno 是 EINPROGRESS 类型时表示连接还在进行 */
    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(sock, &writefds);

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(sock+1, NULL, &writefds, NULL, &timeout);
    if (ret <= 0) {
        printf("connection timeout \n");
        close(sock);
        return -1;
    }

    if (!FD_ISSET(sock, &writefds)) {
        printf("no events on sock found\n");
        close(sock);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);

    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        printf("get socket option failed\n");
        close(sock);
        return -1;
    }

    if (error != 0) {
        perror("Error");
        printf("connection failed after select with error : %d\n", error);
        close(sock);
        return -1;
    }

    printf("connection ready after select with socket: %d\n", sock);
    fcntl(sock, F_SETFL, fdopt);
    return sock;
}
