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
    // struct hostent* hostinfo = gethostbyname(host);
    // assert(hostinfo);

    // struct servent* servinfo = getservbyname("daytime", "tcp");
    // assert(servinfo);

    struct addrinfo hints;
    struct addrinfo* res;

    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo("localhost", "daytime", &hints, &res);

    struct sockaddr_in addr = *(struct sockaddr_in*)res->ai_addr;

    char ipbuf[16];  // INET_ADDRSTRLEN
    printf("ip: %s\n", inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)));
    printf("port: %d\n", ntohs(addr.sin_port));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int result = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    assert(result != -1);

    char buffer[128];
    result = read(sockfd, buffer, sizeof(buffer));
    assert(result > 0);
    buffer[result] = 0;
    printf("the daytime  is :%s", buffer);

    freeaddrinfo(res);
    close(sockfd);
    return 0;
}
