#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

void find(char *path, char *name) {
    struct stat st;
    int fd;
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        exit(0);
    }
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        exit(0);
    }
    char *p, son_path[512];
    switch (st.type) {
        case T_FILE:
            for (p = path + strlen(path); p >= path && *p != '/'; p--)
                ;
            p++;
            if (strcmp(p, name) == 0)
                printf("%s\n", path);

            break;
        case T_DIR:
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(son_path)) {
                printf("find: path too long\n");
                break;
            }
            strcpy(son_path, path);
            p = son_path + strlen(son_path);
            *p++ = '/';
            struct dirent de;
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0)
                    continue;
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(son_path, name);
            }

            break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("error argument num\n");
        exit(0);
    }
    find(argv[1], argv[2]);
    exit(0);
}