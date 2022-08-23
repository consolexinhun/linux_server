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
#include <sys/sendfile.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s file\n", basename(argv[0]));
        return 1;
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    assert(fd > 0);

    int pipefd_stdout[2];
    int ret = pipe(pipefd_stdout);
    assert(ret != -1);

    int pipefd_file[2];
    ret = pipe(pipefd_file);
    assert(ret != -1);

    int len = 32768;
    /* 将标准输入的内容输入到管道 pipefd_stdout */
    ret = splice(STDIN_FILENO, NULL, pipefd_stdout[1], NULL, len, SPLICE_F_MOVE | SPLICE_F_MORE);
    assert(ret != -1);

    /* 将管道 pipefd_stdout 的输出复制到管道  pipefd_file 的输入*/
    ret = tee(pipefd_stdout[0], pipefd_file[1], len, SPLICE_F_NONBLOCK);
    assert(ret != -1);

    /* 将管道 pipefd_file 的输出定向到文件描述符 fd 上，从而将标准输入写入文件 */
    ret = splice(pipefd_file[0], NULL, fd, NULL, len, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert(ret != -1);

    /* 将管道 pipefd_stdout 输出定向到标准输出，内容和写入文件的一样 */
    ret = splice(pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, len, SPLICE_F_MOVE | SPLICE_F_MORE);
    assert(ret != -1);

    close(fd);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);
    close(pipefd_file[0]);
    close(pipefd_file[1]);
    return 0;
}
