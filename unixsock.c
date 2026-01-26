#define _GNU_SOURCE
#include "unixsock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int unixsock_load(unixsock_db_t *db)
{
    memset(db, 0, sizeof(*db));

    FILE *f = fopen("/proc/net/unix", "r");
    if (!f) return -1;

    char line[1024];

    // header
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }

    size_t cap = 128;
    db->v = (unixsock_ent_t *)calloc(cap, sizeof(unixsock_ent_t));
    if (!db->v) {
        fclose(f);
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
      // Num RefCount Protocol Flags Type St Inode Path
      // 0000000000000000: 00000002 00000000 00010000 0001 01 123456 /run/foo.sock
      
        char num[64];
        unsigned int refc = 0, proto = 0;
        unsigned long flags = 0;
        unsigned int type = 0, st = 0;
        unsigned long inode = 0;

        int k = sscanf(line, "%63s %x %x %lx %x %x %lu",
                       num, &refc, &proto, &flags, &type, &st, &inode);
        if (k < 7) continue;

        char *p = strchr(line, '\n');
        if (p) *p = '\0';
      
        char inode_str[32];
        snprintf(inode_str, sizeof(inode_str), "%lu", inode);

        char *pos = strstr(line, inode_str);
        char pathbuf[256] = {0};
        if (pos) {
            pos += strlen(inode_str);
            while (*pos == ' ' || *pos == '\t') pos++;
            // agora pos aponta pro path ou fim da linha
            if (*pos) {
                strncpy(pathbuf, pos, sizeof(pathbuf) - 1);
            }
        }

        if (db->n == cap) {
            cap *= 2;
            unixsock_ent_t *tmp = (unixsock_ent_t *)realloc(db->v, cap * sizeof(unixsock_ent_t));
            if (!tmp) {
                fclose(f);
                free(db->v);
                db->v = NULL;
                db->n = 0;
                return -1;
            }
            db->v = tmp;
        }

        db->v[db->n].inode = inode;
        db->v[db->n].type = type;
        db->v[db->n].state = st;
        strncpy(db->v[db->n].path, pathbuf, sizeof(db->v[db->n].path) - 1);
        db->n++;
    }

    fclose(f);
    return 0;
}

const unixsock_ent_t *unixsock_find(const unixsock_db_t *db, unsigned long inode)
{
    // linear search é OK (geralmente /proc/net/unix tem poucas milhares linhas)
    for (size_t i = 0; i < db->n; i++) {
        if (db->v[i].inode == inode) return &db->v[i];
    }
    return NULL;
}

void unixsock_free(unixsock_db_t *db)
{
    free(db->v);
    db->v = NULL;
    db->n = 0;
}
