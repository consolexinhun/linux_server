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
#include <netdb.h>


int main(int argc, char** argv) {
    struct addrinfo hints;
    struct addrinfo* res;

    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo("localhost", "daytime", &hints, &res);

    char hostbuf[16];
    char servbuf[10];
    // 这里 addr_size 必须先将 sockaddr 转 sockaddr_in，再计算 sizeof
    // 不能直接 sizeof(sockaddr)，因为 IPv6 的 sockaddr 比 IPv4 的长
    // 或者使用 INET_ADDRSTRLEN 及 INET6_ADDRSTRLEN 分别表示 IPv4 和 IPv6 地址长度
    int ret = getnameinfo(res->ai_addr, INET_ADDRSTRLEN, hostbuf, sizeof(hostbuf), servbuf, sizeof(servbuf), NI_NAMEREQD);

    if (ret == 0) {
        printf("host: %s\n", hostbuf);
        printf("serv: %s\n", servbuf);
    } else {
        printf("error is :%s\n", gai_strerror(ret));
    }

    freeaddrinfo(res);
    return 0;
}
