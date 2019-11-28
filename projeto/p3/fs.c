#include "fs.h"
#include "lib/bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sync.h"
#include "lib/hash.h"
#include "lib/inodes.h"


tecnicofs* new_tecnicofs(){
	tecnicofs*fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->bstRoot = malloc(sizeof(node*)*numberBuckets);
	fs->bstLock = malloc(sizeof(syncMech)*numberBuckets);
	if (!fs->bstRoot || !fs->bstLock){
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	for (int index = 0; index<numberBuckets; index++){
		fs->bstRoot[index] = NULL;
		sync_init(&(fs->bstLock[index]));
	}
	inode_table_init();
	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	inode_table_destroy();
	for(int index = 0; index<numberBuckets; index++){
		free_tree(fs->bstRoot[index]);
		sync_destroy(&(fs->bstLock[index]));
	}
	free(fs->bstLock);
	free(fs->bstRoot);
	free(fs);	
}

int create(tecnicofs* fs, char *name,uid_t owner ,permission *perms){
	//Verificar se ficheiro existe
	int index = hash(name, numberBuckets);
	int error_code = 0;
	sync_wrlock(&(fs->bstLock[index]));		//TODO: read lock no search? inumbers seguidos?
	node* searchNode = search(fs->bstRoot[index], name);
	
	//verificacao erros
	if(searchNode)
		error_code = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
	else if(perms[0]==TECNICOFS_ERROR_OTHER || perms[1] == TECNICOFS_ERROR_OTHER)		
		error_code = TECNICOFS_ERROR_OTHER;

	if(error_code){
		sync_unlock(&(fs->bstLock[index]));
		free(perms);
		return error_code;
	}

	//criar inode obter inumber
	int inumber = inode_create(owner, perms[0], perms[1]);
	if(inumber<0)
		return TECNICOFS_ERROR_OTHER;
	
	//insert bst
	fs->bstRoot[index] = insert(fs->bstRoot[index], name, inumber);
	sync_unlock(&(fs->bstLock[index]));
	
	free(perms);
	return 0;
}

int delete(tecnicofs* fs, char *name, uid_t user){
	//Verificar se ficheiro existe
	int index = hash(name, numberBuckets);
	int error_code = 0, aux;
	uid_t owner;
	permission ownerPerm,othersPerm;
	char* fileContents=NULL;
	sync_wrlock(&(fs->bstLock[index]));
	node* searchNode = search(fs->bstRoot[index], name);
	
	if(!searchNode)
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;
	else
		aux = inode_get(searchNode->inumber,&owner,&ownerPerm,&othersPerm,fileContents,0);

	if(error_code);
	else if(aux<0)
		error_code = TECNICOFS_ERROR_OTHER;
	else if(searchNode->isOpen)
		error_code = TECNICOFS_ERROR_FILE_IS_OPEN;
	else if((user==owner && !(ownerPerm & 0b00000001)) && !(othersPerm & 0b00000001))	//0b00000001 = WRITE
		error_code = TECNICOFS_ERROR_PERMISSION_DENIED;

	if(error_code){
		sync_unlock(&(fs->bstLock[index]));
		return error_code;
	}

	inode_delete(searchNode->inumber);
	fs->bstRoot[index] = remove_item(fs->bstRoot[index], name);
	sync_unlock(&(fs->bstLock[index]));
	return error_code;
}

int reName(tecnicofs* fs, char *name, char *newName, int inumber){
	int index0 = hash(name, numberBuckets);
	int index1 = hash(newName, numberBuckets);

	if(index0!=index1)
		for(int i=0, unlocked=TRUE; unlocked; usleep(rand()%100 * MINGUA_CONSTANT*i*i), i++){	//devolve valor  [0, 0.1] * i^2
			unlocked=TRUE;
			if(!sync_try_lock(&(fs->bstLock[index0]))){		//lock primeira arvore
				if(!sync_try_lock(&(fs->bstLock[index1])))	//lock segunda arvore
					unlocked=FALSE;
				else
					sync_unlock(&(fs->bstLock[index0]));	//se bloqueou a segunda e nao a primeira unlocka a primeira
			}
		}
	else
		sync_wrlock(&(fs->bstLock[index1]));

	fs->bstRoot[index0] = remove_item(fs->bstRoot[index0], name);			//remove
	fs->bstRoot[index1] = insert(fs->bstRoot[index1], newName, inumber);	//adiciona
	
	sync_unlock(&(fs->bstLock[index1]));
	if(index0!=index1)
		sync_unlock(&(fs->bstLock[index0]));
	return 0;
}

int lookup(tecnicofs* fs, char *name){
	int inumber = 0;
	int index = hash(name, numberBuckets);
	sync_rdlock(&(fs->bstLock[index]));
	
	node* searchNode = search(fs->bstRoot[index], name);
	if ( searchNode ) {
		inumber = searchNode->inumber;
	}
	sync_unlock(&(fs->bstLock[index]));
	return inumber;
}

int openFile(tecnicofs *fs, char* filename,char* mode){
	int index = hash(name, numberBuckets);
	int error_code = 0, aux;
	uid_t owner;
	permission ownerPerm,othersPerm;
	char* fileContents=NULL;
	sync_wrlock(&(fs->bstLock[index]));
	node* searchNode = search(fs->bstRoot[index], name);

	//Verificar se ficheiro existe
	if(!searchNode)
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;
	else
		aux = inode_get(searchNode->inumber,&owner,&ownerPerm,&othersPerm,fileContents,0);

	if(error_code);
	else if(aux<0)
		error_code = TECNICOFS_ERROR_OTHER;
	else if(searchNode->isOpen)
		error_code = TECNICOFS_ERROR_FILE_IS_OPEN;
	else if((user==owner && !(ownerPerm & 0b00000010)) && !(othersPerm & 0b00000010))	//0b00000001 = WRITE
		error_code = TECNICOFS_ERROR_PERMISSION_DENIED;

	if(error_code){
		sync_unlock(&(fs->bstLock[index]));
		return error_code;
	}

	searchNode -> nUsers++;
	searchNode -> users = realloc(searchNode -> users, sizeof(uid_t) * searchNode -> nUsers)
	cliente -> 
	sync_unlock(&(fs->bstLock[index]));
	return error_code;

}



int closeFile(tecnicofs *fs, char* filename){return 0;}
int writeToFile(tecnicofs *fs, char* filename, char* dataInBuffer){return 0;}
int readFromFile(tecnicofs *fs, char* filename, char* len){return 0;}


void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	for (int index = 0; index < numberBuckets; index++){
		print_tree(fp, fs->bstRoot[index]);
	}
}
