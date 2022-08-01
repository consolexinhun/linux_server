#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;
static void handle_term(int dig) { stop = true; }

int main(int argc, char** argv) {
    signal(SIGTERM, handle_term);

    if (argc <= 3) {
        printf("usage: %s ip port backlog\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int ret = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(sock, backlog);
    assert(ret != -1);

    while (!stop) {
        sleep(1);
    }

    close(sock);

    return 0;
}
