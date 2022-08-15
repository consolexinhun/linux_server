#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

const int BUFFER = 1024;

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
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock > 0);

    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);

    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, reinterpret_cast<socklen_t*>(&len));
    printf("the tcp recv buffer size after setting is %d\n", recvbuf);

    int ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_size = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_size);

    if (connfd < 0) {
        printf("errno is %d\n", errno);
    } else {
        char buffer[BUFFER];
        memset(buffer, 0, BUFFER);
        while (recv(connfd, buffer, BUFFER-1, 0) > 0) { }
        close(connfd);
    }
    close(sock);

    return 0;
}
