// minilsof.c - MVP: lista FDs via /proc/<pid>/fd (tipo, mode, size, target)
#define _GNU_SOURCE
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int read_fdinfo(pid_t pid, int fd,
                unsigned long long *pos_out,
                unsigned long *flags_out);

const char *accmode_str(unsigned long flags);

typedef struct {
    int fd;
    char name[32];
} fdent_t;

void die(const char *msg) {
    perror(msg);
    exit(1);
}

int cmp_fdent(const void *a, const void *b) {
    const fdent_t *x = (const fdent_t *)a;
    const fdent_t *y = (const fdent_t *)b;
    return (x->fd > y->fd) - (x->fd < y->fd);
}

const char *ftype_from_mode(mode_t m) {
    if (S_ISREG(m))  return "REG";
    if (S_ISDIR(m))  return "DIR";
    if (S_ISCHR(m))  return "CHR";
    if (S_ISBLK(m))  return "BLK";
    if (S_ISFIFO(m)) return "FIFO";
    if (S_ISSOCK(m)) return "SOCK";
    if (S_ISLNK(m))  return "LNK";
    return "?";
}

void list_fds(pid_t pid) {
    char fd_dir[64];
    snprintf(fd_dir, sizeof(fd_dir), "/proc/%d/fd", pid);

    DIR *d = opendir(fd_dir);
    if (!d) die("opendir /proc/<pid>/fd");

    // 1) coleta entradas
    size_t cap = 64, n = 0;
    fdent_t *arr = (fdent_t *)calloc(cap, sizeof(fdent_t));
    if (!arr) die("calloc");

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char *end = NULL;
        long v = strtol(ent->d_name, &end, 10);
        if (!end || *end != '\0' || v < 0 || v > INT_MAX) continue;

        if (n == cap) {
            cap *= 2;
            fdent_t *tmp = (fdent_t *)realloc(arr, cap * sizeof(fdent_t));
            if (!tmp) die("realloc");
            arr = tmp;
        }
        arr[n].fd = (int)v;
        strncpy(arr[n].name, ent->d_name, sizeof(arr[n].name) - 1);
        arr[n].name[sizeof(arr[n].name) - 1] = '\0';
        n++;
    }
    closedir(d);

    qsort(arr, n, sizeof(fdent_t), cmp_fdent);

    printf("PID %d\n", pid);
    printf("%-4s %-5s %-6s %-10s %s\n", "FD", "TYPE", "MODE", "SIZE", "TARGET");

    // 2) imprime
    for (size_t i = 0; i < n; i++) {
        char linkpath[128];
        snprintf(linkpath, sizeof(linkpath), "%s/%s", fd_dir, arr[i].name);

        char target[PATH_MAX];
        ssize_t rl = readlink(linkpath, target, sizeof(target) - 1);
        if (rl < 0) {
            printf("%-4d %-5s %-6s %-10s %s\n",
                   arr[i].fd, "?", "????", "?", strerror(errno));
            continue;
        }
        target[rl] = '\0';

        struct stat st;
        const char *type = "?";
        char modebuf[8] = "????";
        char sizebuf[32] = "?";

        if (stat(linkpath, &st) == 0) {
            type = ftype_from_mode(st.st_mode);
            snprintf(modebuf, sizeof(modebuf), "%04o", (unsigned)(st.st_mode & 07777));

            // size faz sentido p/ REG; p/ outros deixa como número mesmo (é ok)
            snprintf(sizebuf, sizeof(sizebuf), "%lld", (long long)st.st_size);
        } else {
            // não conseguiu stat (ex.: perm, corrida), ainda mostra o target
            type = "?";
        }

        printf("%-4d %-5s %-6s %-10s %s\n",
               arr[i].fd, type, modebuf, sizebuf, target);
    }

    free(arr);
}

pid_t parse_pid(const char *s) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0' || v <= 0 || v > INT32_MAX) return -1;
    return (pid_t)v;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <pid>\n", argv[0]);
        return 2;
    }
    pid_t pid = parse_pid(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "invalid pid: %s\n", argv[1]);
        return 2;
    }
    list_fds(pid);
    return 0;
}
