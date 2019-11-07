#include "fs.h"
#include "lib/bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sync.h"
#include "lib/hash.h"


int obtainNewInumber(tecnicofs* fs) {//como os inumbers são independentes do bucket gravados em fs[0]
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

tecnicofs* new_tecnicofs(){
	tecnicofs*fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->nextINumber = 0;
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
	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	for(int index = 0; index<numberBuckets; index++){
		free_tree(fs->bstRoot[index]);
		sync_destroy(&(fs->bstLock[index]));
	}
	free(fs->bstLock);
	free(fs->bstRoot);
	free(fs);
}

void create(tecnicofs* fs, char *name, int inumber){
	int index = hash(name, numberBuckets);
	sync_wrlock(&(fs->bstLock[index]));
	fs->bstRoot[index] = insert(fs->bstRoot[index], name, inumber);
	sync_unlock(&(fs->bstLock[index]));
}

void delete(tecnicofs* fs, char *name){
	int index = hash(name, numberBuckets);
	sync_wrlock(&(fs->bstLock[index]));
	fs->bstRoot[index] = remove_item(fs->bstRoot[index], name);
	sync_unlock(&(fs->bstLock[index]));
}

void reName(tecnicofs* fs, char *name, char *newName, int inumber){
	int index0 = hash(name, numberBuckets);
	int index1 = hash(newName, numberBuckets);

	if(index0!=index1)
		for(int i=0, unlocked=TRUE; unlocked; usleep(MINGUA_CONSTANT*i), i++){
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

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	for (int index = 0; index < numberBuckets; index++){
		print_tree(fp, fs->bstRoot[index]);
	}
}
