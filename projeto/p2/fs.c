#include "fs.h"
#include "lib/bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sync.h"
#include "lib/hash.h"


int obtainNewInumber(tecnicofs** fs) {//como os inumbers são independentes do bucket gravados em fs[0]
	int newInumber = ++(fs[0]->nextINumber);
	return newInumber;
}

tecnicofs** new_tecnicofs(){
	tecnicofs**hashtable = malloc(numberBuckets * sizeof(tecnicofs*));
	for (int i = 0; i<numberBuckets; i++){
		tecnicofs*fs = malloc(sizeof(tecnicofs));
		if (!fs) {
			perror("failed to allocate tecnicofs");
			exit(EXIT_FAILURE);
		}
		fs->bstRoot = NULL;
		fs->nextINumber = 0;
		sync_init(&(fs->bstLock));
		hashtable[i]=fs;
	}
	return hashtable;
}

void free_tecnicofs(tecnicofs** fs){
	for(int i = 0; i<numberBuckets; i++){
		free_tree(fs[i]->bstRoot);
		sync_destroy(&(fs[i]->bstLock));
		free(fs[i]);
	}
	free(fs);
}

void create(tecnicofs** fs, char *name, int inumber){
	int hashPlace = hash(name, numberBuckets);
	sync_wrlock(&(fs[hashPlace]->bstLock));
	fs[hashPlace]->bstRoot = insert(fs[hashPlace]->bstRoot, name, inumber);
	sync_unlock(&(fs[hashPlace]->bstLock));
}

void delete(tecnicofs** fs, char *name){
	int hashPlace = hash(name, numberBuckets);
	sync_wrlock(&(fs[hashPlace]->bstLock));
	fs[hashPlace]->bstRoot = remove_item(fs[hashPlace]->bstRoot, name);
	sync_unlock(&(fs[hashPlace]->bstLock));
}

int lookup(tecnicofs** fs, char *name){
	int inumber = 0;
	int hashPlace = hash(name, numberBuckets);
	sync_rdlock(&(fs[hashPlace]->bstLock));
	
	node* searchNode = search(fs[hashPlace]->bstRoot, name);
	if ( searchNode ) {
		inumber = searchNode->inumber;
	}
	sync_unlock(&(fs[hashPlace]->bstLock));
	return inumber;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs **fs){
	for (int i = 0; i < numberBuckets; i++){
		print_tree(fp, fs[i]->bstRoot);
	}
}
