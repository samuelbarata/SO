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
	for (int i = 0; i<numberBuckets; i++){
		fs->bstRoot[i] = NULL;
		sync_init(&(fs->bstLock[i]));
	}
	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	for(int i = 0; i<numberBuckets; i++){
		free_tree(fs->bstRoot[i]);
		sync_destroy(&(fs->bstLock[i]));
	}
	free(fs->bstLock);
	free(fs->bstRoot);
	free(fs);
}

void create(tecnicofs* fs, char *name, int inumber){
	int hashPlace = hash(name, numberBuckets);
	sync_wrlock(&(fs->bstLock[hashPlace]));
	fs->bstRoot[hashPlace] = insert(fs->bstRoot[hashPlace], name, inumber);
	sync_unlock(&(fs->bstLock[hashPlace]));
}

void delete(tecnicofs* fs, char *name){
	int hashPlace = hash(name, numberBuckets);
	sync_wrlock(&(fs->bstLock[hashPlace]));
	fs->bstRoot[hashPlace] = remove_item(fs->bstRoot[hashPlace], name);
	sync_unlock(&(fs->bstLock[hashPlace]));
}

int lookup(tecnicofs* fs, char *name){
	int inumber = 0;
	int hashPlace = hash(name, numberBuckets);
	sync_rdlock(&(fs->bstLock[hashPlace]));
	
	node* searchNode = search(fs->bstRoot[hashPlace], name);
	if ( searchNode ) {
		inumber = searchNode->inumber;
	}
	sync_unlock(&(fs->bstLock[hashPlace]));
	return inumber;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	for (int i = 0; i < numberBuckets; i++){
		print_tree(fp, fs->bstRoot[i]);
	}
}
