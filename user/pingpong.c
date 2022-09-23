#include "kernel/types.h"
#include "user.h"

// int main(int argc, char *argv[]) {
//     int p[2];
//     pipe(p);  // p[0]读，p[1]写
//     int ret = fork();
//     if (ret > 0) {  // 父进程
//         char buf[5];
//         write(p[1], "ping", 5);
//         sleep(5);
//         read(p[0], buf, 5);
//         printf("%d: received %s\n", getpid(), buf);
//         close(p[0]);
//         close(p[1]);

//         wait((int *)0);
//     } else if (ret == 0) {  // 子进程
//         char buf[5];
//         write(p[1], "pong", 5);
//         read(p[0], buf, 5);
//         printf("%d: received %s\n", getpid(), buf);
//         close(p[0]);
//         close(p[1]);
//     } else
//         printf("fork error\n");
//     exit(0);
// }

int main(int argc, char *argv[]) {
    int p[2], q[2];
    pipe(p);  // p[0]读，p[1]写
    pipe(q);
    int ret = fork();
    if (ret > 0) {  // 父进程
        close(p[0]);
        close(q[1]);
        char buf[5];
        write(p[1], "ping", 5);
        read(q[0], buf, 5);
        printf("%d: received %s\n", getpid(), buf);
        close(p[1]);
        close(q[0]);

        wait((int *)0);
    } else if (ret == 0) {  // 子进程
        close(p[1]);
        close(q[0]);
        char buf[5];
        read(p[0], buf, 5);
        printf("%d: received %s\n", getpid(), buf);
        write(q[1], "pong", 5);
        close(p[0]);
        close(q[1]);
    } else
        printf("fork error\n");
    exit(0);
}