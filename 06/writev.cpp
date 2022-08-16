#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/uio.h>

const int BUFFER = 1024;

static const char* status_line[2] = {
    "200 OK",
    "500 Interal server error"
};

int main(int argc, char** argv) {
    if (argc <= 3) {
        printf("usage: %s ip port filename\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* filename = argv[3];

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

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
        char header_buf[BUFFER];
        memset(header_buf, 0, sizeof(header_buf));
        char *file_buf;
        struct stat file_stat;
        bool valid = true;
        int len = 0;

        if (stat(filename, &file_stat) < 0) {  // 目标文件不存在
            valid = false;
        } else {
            if (S_ISDIR(file_stat.st_mode)) {
                valid = false;
            } else if (file_stat.st_mode & S_IROTH) {  // 当前用户具有读的权限
                /* 
                动态分配缓存区 file_buf 并指定其大小为目标文件大小，file_stat.st_size + 1再将目标文件读入缓存区 file_buf 中
                 */
                int fd = open(filename, O_RDONLY);
                file_buf = new char[file_stat.st_size + 1];
                memset(file_buf, 0, sizeof(file_buf));
                if (read(fd, file_buf, file_stat.st_size) < 0) {
                    valid = false;
                }
            } else {
                valid = false;
            }
        }
        /* 
        如果目标文件有效，那么发送正常的 HTTP 应答
         */
        if (valid) {
            ret = snprintf(header_buf, BUFFER-1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER-1-len, "Content-Length: %d\r\n", file_stat.st_size);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER-1-len, "%s", "\r\n");

            struct iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);

            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;

            ret = writev(connfd, iv, 2);
        } else {
            ret = snprintf(header_buf, BUFFER-1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
            len += ret;
            ret = snprintf(header_buf + len, BUFFER-1-len, "%s", "\r\n");
            send(connfd, header_buf, strlen(header_buf), 0);
        }
        close(connfd);
        delete[] file_buf;
    }
    close(sock);
    return 0;
}
