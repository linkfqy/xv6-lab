#include "kernel/types.h"
#include "user.h"

#define MAXN 35
void sieve(int old_fd) {
    int p;
    read(old_fd, &p, sizeof(p));
    printf("prime %d\n", p);

    int fd[2], x, flg = 0;
    pipe(fd);
    // sieve by p
    while (read(old_fd, &x, sizeof(x))) {
        if (x % p != 0)
            write(fd[1], &x, sizeof(x));
        flg = 1;
    }
    close(old_fd);
    if (!flg)
        exit(0);

    int ret = fork();
    if (ret > 0) {
        close(fd[0]);
        close(fd[1]);
        wait((int *)0);
    } else if (ret == 0) {
        close(fd[1]);
        sieve(fd[0]);
    } else
        printf("fork error\n");

    exit(0);
}

int main(int argc, char *argv[]) {
    int p[2];
    pipe(p);
    int ret = fork();
    if (ret > 0) {  // father
        close(p[0]);
        for (int i = 2; i <= MAXN; i++)
            write(p[1], &i, sizeof(i));
        close(p[1]);

        wait((int *)0);
    } else if (ret == 0) {  // child
        close(p[1]);
        sieve(p[0]);
    }
    exit(0);
}