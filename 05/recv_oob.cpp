#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

const int BUF = 1024;

int main(int argc, char** argv) {
    if (argc <= 2) {
        printf("usage: %s ip port\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_port = port;
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr));

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_size = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_size);
    if (connfd < 0) {
        printf("errno is : %d\n", errno);
    } else {
        char buff[BUF];
        memset(buff, 0, BUF);
        ret = recv(connfd, buff, BUF - 1, 0);
        printf("got %d bytes of normal data '%s' \n", ret, buff);

        memset(buff, 0, BUF);
        ret = recv(connfd, buff, BUF - 1, MSG_OOB);
        printf("got %d bytes of oob data '%s'\n", ret, buff);

        memset(buff, 0, BUF);
        ret = recv(connfd, buff, BUF - 1, 0);
        printf("got %d bytes of normal data  '%s'\n", ret, buff);
        close(connfd);
    }
    close(sock);
}
