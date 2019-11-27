/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include "sync.h"
#include "globals.h"
#include "tecnicofs-api-constants.h"

typedef struct tecnicofs {
    node** bstRoot;
    syncMech* bstLock;
} tecnicofs;

extern int numberBuckets;

tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
int create(tecnicofs *fs, char *name,uid_t owner, permission *perms);
int delete(tecnicofs *fs, char *name,uid_t user);
int reName(tecnicofs* fs, char *name, char *newName, int inumber);
int lookup(tecnicofs *fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);
int openFile(tecnicofs *fs, char* filename,char* mode);
int closeFile(tecnicofs *fs, char* filename);
int writeToFile(tecnicofs *fs, char* filename, char* dataInBuffer);
int readFromFile(tecnicofs *fs, char* filename, char* len);

#endif /* FS_H */
