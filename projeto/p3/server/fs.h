/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef FS_H
#define FS_H
#include "../lib/bst.h"
#include "../lib/sync.h"
#include "../globals.h"
#include "../tecnicofs-api-constants.h"

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
int reName(tecnicofs* fs, char *name, char *newName, client* user);
int lookup(tecnicofs *fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);
int openFile(tecnicofs *fs, char* filename,char* mode, client* user);
int closeFile(tecnicofs *fs, char* filename, client* user);
int writeToFile(tecnicofs *fs, char* filename, char* dataInBuffer, client* user);
char* readFromFile(tecnicofs *fs, char* filename, char* len, client* user);
permission *permConv(char* perms);    //recebe string com permissões; devolve array int [owner, others]
int checkUserPerms(client* , int , int, char*, int);
int ficheiroApagadoChecker(tecnicofs *fs, client *user, int fd, int checker);

#endif /* FS_H */
