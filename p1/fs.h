#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include <sys/types.h>

typedef struct tecnicofs {
    node* bstRoot;
    int nextINumber;
    pthread_mutex_t treeMutexLock;
    pthread_rwlock_t treeRwLock;
} tecnicofs;

int obtainNewInumber(tecnicofs* fs);
tecnicofs** new_tecnicofs();
void free_tecnicofs(tecnicofs** fs);
void create(tecnicofs** fs, char *name, int inumber);
void delete(tecnicofs** fs, char *name);
int lookup(tecnicofs** fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs **fs);

#endif /* FS_H */

//testar erro outoput file;
//numthreads
//erro do gettime
//erro do join
//nomes locks
