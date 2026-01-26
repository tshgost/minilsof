#ifndef UNIXSOCK_H
#define UNIXSOCK_H

#include <stddef.h>

typedef struct {
    unsigned long inode;
    char path[256];      // pode ser vazio
    unsigned int type;   // do /proc/net/unix (1=STREAM, 2=DGRAM, etc)
    unsigned int state;  // "St"
} unixsock_ent_t;

typedef struct {
    unixsock_ent_t *v;
    size_t n;
} unixsock_db_t;

int unixsock_load(unixsock_db_t *db);

const unixsock_ent_t *unixsock_find(const unixsock_db_t *db, unsigned long inode);

void unixsock_free(unixsock_db_t *db);

#endif
