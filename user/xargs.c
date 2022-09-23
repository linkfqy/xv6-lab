#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"
#define BUFSIZE 1024
/*
xargs cmd arg0 arg1 arg2 0
0     1   2    3    4    5
      pr0 pr1  pr2  pr3
argc=5
argn=4
*/
char whitespace[] = " \t\r\n\v";
int main(int argc, char *argv[]) {
    char buf[BUFSIZE];
    char *cmd = argv[1], *paras[MAXARG];
    int argn = argc - 1;
    for (int i = 1; i < argc; i++)
        paras[i - 1] = argv[i];
    // while buf is not empty
    while (*gets(buf, BUFSIZE)) {
        int n = argn, flag = 0;  // last char blank-0/not blank-1
        for (int i = 0; buf[i]; i++)
            if (strchr(whitespace, buf[i])) {
                buf[i] = 0;
                flag = 0;
            } else {
                if (!flag) {
                    // ignore arguments out of MAXARG
                    if (n + 1 >= MAXARG)
                        break;
                    paras[n++] = buf + i;
                }
                flag = 1;
            }
        paras[n] = 0;
        int ret = fork();
        if (ret > 0)
            wait((int *)0);
        else if (ret == 0)
            exec(cmd, paras);
        else {
            printf("fork error\n");
            exit(0);
        }
    }
    exit(0);
}