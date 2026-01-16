#include <fcntl.h>   // O_* flags

static int read_fdinfo(pid_t pid, int fd,
                       unsigned long long *pos_out,
                       unsigned long *flags_out)
{
    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/fdinfo/%d", pid, fd);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    int have_pos = 0, have_flags = 0;

    while (fgets(line, sizeof(line), f)) {
        unsigned long long pos;
        unsigned long flags;
        if (!have_pos && sscanf(line, "pos:\t%llu", &pos) == 1) {
            *pos_out = pos;
            have_pos = 1;
        } else if (!have_flags && sscanf(line, "flags:\t%lo", &flags) == 1) {
            *flags_out = flags;
            have_flags = 1;
        }
        if (have_pos && have_flags) break;
    }

    fclose(f);
    return (have_pos && have_flags) ? 0 : 1;
}

static const char *accmode_str(unsigned long flags) {
    switch (flags & O_ACCMODE) {
        case O_RDONLY: return "RDONLY";
        case O_WRONLY: return "WRONLY";
        case O_RDWR:   return "RDWR";
        default:       return "?";
    }
}
