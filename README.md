# minilsof

A minimal `lsof`-like tool for Linux that lists open file descriptors (FDs) for a given process using `/proc/<pid>/fd`.

This is a learning/tooling project focused on Linux process introspection.

## Features
- Lists FDs for a target PID
- Shows:
  - FD number
  - file type (REG/DIR/CHR/BLK/FIFO/SOCK/LNK)
  - mode (octal)
  - size (from `stat`)
  - access mode (RDONLY/WRONLY/RDWR) and file position (POS) via `/proc/<pid>/fdinfo/<fd>`
  - symlink target (e.g. `/dev/pts/2`, `pipe:[...]`, `socket:[...]`)

## Build
Requires Linux with `/proc` (native Linux or a Linux VM).

```bash
gcc -std=c11 -O2 -Wall -Wextra -pedantic minilsof.c fdinfo.c -o minilsof
