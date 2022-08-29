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
#include <poll.h>

#define BUF 64
#define USER_LIMIT 5
#define FD_LIMIT 65535

struct client_data {
    sockaddr_in addr;
    char* write_buf;
    char buf[BUF];
};

int setnonblock(int fd);

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

    int server = socket(PF_INET, SOCK_STREAM, 0);
    int ret = bind(server, (struct sockaddr*)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(server, 5);
    assert(ret != -1);

    client_data* users = new client_data[FD_LIMIT];
    pollfd fds[USER_LIMIT + 1];
    int user_cnt = 0;
    for (int i = 1; i <= USER_LIMIT; i++) {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    fds[0].fd = server;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while (1) {
        ret = poll(fds, user_cnt+1, -1);
        if (ret < 0) {
            printf("poll failure\n");
            break;
        }
        for (int i = 0; i < user_cnt+1; i++) {
            if (fds[i].fd == server && fds[i].revents & POLLIN) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);
                int client = accept(server, (struct sockaddr*)&client_addr, &client_addr_size);
                if (client < 0) {
                    perror("accept() error");
                    continue;
                }

                if (user_cnt >= USER_LIMIT) {
                    const char* info = "too many users\n";
                    printf("%s", info);
                    send(client, info, strlen(info), 0);
                    close(client);
                    continue;
                }

                user_cnt++;
                users[client].addr = client_addr;
                setnonblock(client);
                fds[user_cnt].fd = client;
                fds[user_cnt].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_cnt].revents = 0;
                printf("comes a new user, now have %d users\n", user_cnt);
            } else if (fds[i].revents & POLLERR) {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, 0, 100);
                socklen_t len = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &len) < 0) {
                    printf("get socket option failed\n");
                }
                continue;
            } else if (fds[i].revents & POLLRDHUP) {
                users[fds[i].fd] = users[fds[user_cnt].fd];
                close(fds[i].fd);
                fds[i] = fds[user_cnt];
                i--;
                user_cnt--;
                printf("a client left\n");
            } else if (fds[i].revents & POLLIN) {
                int client = fds[i].fd;
                memset(users[client].buf, 0, BUF);
                ret = recv(client, users[client].buf, BUF-1, 0);
                printf("get %d bytes of client data : %s from %d\n", ret, users[client].buf, client);

                if (ret < 0) {
                    if (errno != EAGAIN) {
                        close(client);
                        users[fds[i].fd] = users[fds[user_cnt].fd];
                        fds[i] = fds[user_cnt];
                        i--;
                        user_cnt--;
                    }
                } else if (ret == 0) {
                    printf("code should not come to here\n");
                } else {
                    for (int j = 1; j <= user_cnt; j++) {
                        if (fds[j].fd == client) {
                            continue;
                        }
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[client].buf;
                    }
                }
            } else if (fds[i].revents & POLLOUT) {
                int client = fds[i].fd;
                if (!users[client].write_buf) {
                    continue;
                }

                ret = send(client, users[client].write_buf, strlen(users[client].write_buf), 0);
                users[client].write_buf = NULL;
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete[] users;
    close(server);
    return 0;
}

int setnonblock(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
