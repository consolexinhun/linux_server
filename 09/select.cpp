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

int main(int argc, char* argv[] ) {
    if (argc <= 2) {
        printf("usage: %s ip port\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // addr.sin_addr.s_addr = inet_addr(ip);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    int connfd = accept(sock, (struct sockaddr*)&client_addr, &client_addr_size);
    if (connfd < 0) {
        perror("accept() error");
        close(connfd);
        exit(1);
    } else {
        puts("ok");
    }

    // char buf[1024];
    // fd_set read_fds;
    // fd_set exception_fds;
    // FD_ZERO(&read_fds);
    // FD_ZERO(&exception_fds);

    // while (1) {
    //     memset(buf, 0, sizeof(buf));
    //     /* 每次调用 select 函数之前都要重新在 read_fds exception_dfs 中设置文件描述符 connfd
    //         因为事件发生后，文件描述符集合会被内核修改
    //      */
    //     FD_SET(connfd, &read_fds);
    //     FD_SET(connfd, &exception_fds);

    //     ret = select(connfd+1, &read_fds, NULL, &exception_fds, NULL);
    //     if (ret < 0) {
    //         printf("select fail\n");
    //         break;
    //     }

    //     if (FD_ISSET(connfd, &read_fds)) {
    //         ret = recv(connfd, buf, sizeof(buf)-1, 0);
    //         if (ret <= 0) {
    //             break;
    //         }
    //         printf("get %d bytes of normal data : %s\n", ret, buf);
    //     } else if (FD_ISSET(connfd, &exception_fds)) {
    //         ret = recv(connfd, buf, sizeof(buf)-1, MSG_OOB);
    //         if (ret <= 0) {
    //             break;
    //         }
    //         printf("get %d bytes of oob data: %s\n", ret, buf);
    //     }
    // }

    close(connfd);
    close(sock);
    return 0;
}
