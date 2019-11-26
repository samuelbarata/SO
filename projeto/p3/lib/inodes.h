#ifndef INODES_H
#define INODES_H

#include <sys/types.h>
#include "../tecnicofs-api-constants.h"

#define FREE_INODE -1
#define INODE_TABLE_SIZE 50
#define _GNU_SOURCE

typedef struct inode_t {
    uid_t owner;
    permission ownerPermissions;
    permission othersPermissions;
    char* fileContent;
} inode_t;


void inode_table_init();
void inode_table_destroy();
int inode_create(uid_t owner, permission ownerPerm, permission othersPerm);
int inode_delete(int inumber);
int inode_get(int inumber,uid_t *owner, permission *ownerPerm, permission *othersPerm,
                     char* fileContents, int len);
int inode_set(int inumber, char *contents, int len);

permission *permConv(char* perms);    //recebe string com permissões; devolve array int [owner, others]

#endif /* INODES_H */