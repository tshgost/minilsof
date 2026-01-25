#ifndef FDINFO_H
#define FDINFO_H

#include <sys/types.h>

int read_fdinfo_pos_flags(pid_t pid, int fd,
                          unsigned long long *pos_out,
                          unsigned long *flags_out);

const char *accmode_str(unsigned long flags);

#endif
