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
extern client* clients[MAX_CLIENTS];

tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
int create(tecnicofs *fs, char *name,client* owner, permission *perms);
int delete(tecnicofs *fs, char *name,client* user);
int reName(tecnicofs* fs, char *name, char *newName, client* user); //, client* user
int lookup(tecnicofs *fs, char *name); //, client* user
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);
int openFile(tecnicofs *fs, char* filename,char* mode, client* user); //, client* user
int closeFile(tecnicofs *fs, char* filename, client* user);
int writeToFile(tecnicofs *fs, char* filename, char* dataInBuffer, client* user);
int readFromFile(tecnicofs *fs, char* filename, char* len, client* user);
int checkUserPerms(client* , node*);

#endif /* FS_H */
