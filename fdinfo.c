// minilsof.c - MVP: lista FDs via /proc/<pid>/fd (tipo, mode, size, target) + ACC/POS via fdinfo
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

typedef struct {
    int fd;
    char name[32];
} fdent_t;

/* funções que vivem em fdinfo.c */
int read_fdinfo_pos_flags(pid_t pid, int fd,
                          unsigned long long *pos_out,
                          unsigned long *flags_out);
const char *accmode_str(unsigned long flags);

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static int cmp_fdent(const void *a, const void *b) {
    const fdent_t *x = (const fdent_t *)a;
    const fdent_t *y = (const fdent_t *)b;
    return (x->fd > y->fd) - (x->fd < y->fd);
}

static const char *ftype_from_mode(mode_t m) {
    if (S_ISREG(m))  return "REG";
    if (S_ISDIR(m))  return "DIR";
    if (S_ISCHR(m))  return "CHR";
    if (S_ISBLK(m))  return "BLK";
    if (S_ISFIFO(m)) return "FIFO";
    if (S_ISSOCK(m)) return "SOCK";
    if (S_ISLNK(m))  return "LNK";
    return "?";
}

static void list_fds(pid_t pid) {
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
        snprintf(arr[n].name, sizeof(arr[n].name), "%s", ent->d_name);
        n++;
    }
    closedir(d);

    qsort(arr, n, sizeof(fdent_t), cmp_fdent);

    printf("PID %d\n", pid);
    printf("%-4s %-5s %-6s %-10s %-8s %-10s %s\n",
           "FD", "TYPE", "MODE", "SIZE", "ACC", "POS", "TARGET");

    // 2) imprime
    for (size_t i = 0; i < n; i++) {
        char linkpath[128];
        snprintf(linkpath, sizeof(linkpath), "%s/%s", fd_dir, arr[i].name);

        // resolve target do symlink
        char target[PATH_MAX];
        ssize_t rl = readlink(linkpath, target, sizeof(target) - 1);
        if (rl < 0) {
            // sem permissão, corrida com processo, etc.
            printf("%-4d %-5s %-6s %-10s %-8s %-10s %s\n",
                   arr[i].fd, "?", "????", "?", "?", "?", strerror(errno));
            continue;
        }
        target[rl] = '\0';

        // tenta inferir tipo via stat no próprio linkpath (segue o alvo)
        struct stat st;
        const char *type = "?";
        char modebuf[8] = "????";
        char sizebuf[32] = "?";

        if (stat(linkpath, &st) == 0) {
            type = ftype_from_mode(st.st_mode);
            snprintf(modebuf, sizeof(modebuf), "%04o", (unsigned)(st.st_mode & 07777));
            snprintf(sizebuf, sizeof(sizebuf), "%lld", (long long)st.st_size);
        }

        // lê fdinfo (pos/flags)
        unsigned long long pos = 0;
        unsigned long flags = 0;
        int rc = read_fdinfo_pos_flags(pid, arr[i].fd, &pos, &flags);

        const char *acc = (rc == 0) ? accmode_str(flags) : "?";
        char posbuf[32];
        if (rc == 0) snprintf(posbuf, sizeof(posbuf), "%llu", pos);
        else snprintf(posbuf, sizeof(posbuf), "?");

        printf("%-4d %-5s %-6s %-10s %-8s %-10s %s\n",
               arr[i].fd, type, modebuf, sizebuf, acc, posbuf, target);
    }

    free(arr);
}

static pid_t parse_pid(const char *s) {
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
