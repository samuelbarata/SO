#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/hash.c"
#include "definer.h"

int obtainNewInumber(tecnicofs* fs) {
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

tecnicofs** new_tecnicofs(){
	tecnicofs** hashtable = malloc(numberBuckets * sizeof(tecnicofs*));
	for (int i = 0; i < numberBuckets; i++){
		tecnicofs*fs = malloc(sizeof(tecnicofs));
		if (!fs) {
			perror("failed to allocate tecnicofs");
			exit(EXIT_FAILURE);
		}
		fs->bstRoot = NULL;
		fs->nextINumber = 0;
		hashtable[i] = fs;
	}
	return hashtable;
}

void free_tecnicofs(tecnicofs** fs){
	for(int i = 0; i<numberBuckets; i++){
		free_tree(fs[i]->bstRoot);
	}
	free(fs);
}

void create(tecnicofs** fs, char *name, int inumber){
	int hashPlace = hash(name, numberBuckets);
	fs[hashPlace]->bstRoot = insert(fs[hashPlace]->bstRoot, name, inumber);
}

void delete(tecnicofs** fs, char *name){
	int hashPlace = hash(name, numberBuckets);
	fs[hashPlace]->bstRoot = remove_item(fs[hashPlace]->bstRoot, name);
}

int lookup(tecnicofs** fs, char *name){
	int hashPlace = hash(name, numberBuckets);
	node* searchNode = search(fs[hashPlace]->bstRoot, name);
	if ( searchNode ) return searchNode->inumber;
	return 0;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs **fs){
	for (int i = 0; i < numberBuckets; i++){
		print_tree(fp, fs[i]->bstRoot);
	}
}
