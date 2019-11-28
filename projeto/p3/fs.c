#include "fs.h"
#include "lib/bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sync.h"
#include "lib/hash.h"
#include "lib/inodes.h"

client* clients[MAX_CLIENTS];      //array clients

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

int create(tecnicofs* fs, char *name,client *user ,permission *perms){
	//Verificar se ficheiro existe
	int index = hash(name, numberBuckets);
	int error_code = 0;
	sync_wrlock(&(fs->bstLock[index]));
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
	int inumber = inode_create(user->uid, perms[0], perms[1]);
	if(inumber<0)
		return TECNICOFS_ERROR_OTHER;
	
	//insert bst
	fs->bstRoot[index] = insert(fs->bstRoot[index], name, inumber);
	sync_unlock(&(fs->bstLock[index]));
	
	free(perms);
	return 0;
}

int delete(tecnicofs* fs, char *name, client *user){
	//Verificar se ficheiro existe
	int index = hash(name, numberBuckets);
	int error_code = 0;
	uid_t owner;
	permission ownerPerm,othersPerm;
	int extendedPermissions;
	char* fileContents=NULL;
	sync_wrlock(&(fs->bstLock[index]));
	
	node* searchNode = search(fs->bstRoot[index], name);
	if(!searchNode)
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;
	
	if(!error_code && inode_get(searchNode->inumber,&owner,&ownerPerm,&othersPerm,fileContents,0)<0){
		error_code = TECNICOFS_ERROR_OTHER;
	}
	if(!error_code)
		extendedPermissions = checkUserPerms(user , searchNode);
	if(!(extendedPermissions & USER_CAN_WRITE) && !error_code)
		error_code = TECNICOFS_ERROR_PERMISSION_DENIED;

	if((extendedPermissions & OPEN_OTHER_READ || extendedPermissions & OPEN_OTHER_READ ) && !error_code)
		error_code = TECNICOFS_ERROR_FILE_IS_OPEN;

	if(error_code){
		sync_unlock(&(fs->bstLock[index]));
		return error_code;
	}

	inode_delete(searchNode->inumber);
	fs->bstRoot[index] = remove_item(fs->bstRoot[index], name);
	sync_unlock(&(fs->bstLock[index]));
	return error_code;
}

int reName(tecnicofs* fs, char *name, char *newName, client* user){
	int index0 = hash(name, numberBuckets);
	int index1 = hash(newName, numberBuckets);
	node* searchNode = search(fs->bstRoot[index0], name);

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
	fs->bstRoot[index1] = insert(fs->bstRoot[index1], newName, searchNode->inumber);	//adiciona
	
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

int openFile(tecnicofs *fs, char* filename,char* modeIn, client* user){
	int index = hash(filename, numberBuckets);
	int error_code = 0;
	int extendedPermissions;
	sync_wrlock(&(fs->bstLock[index]));
	node* searchNode = search(fs->bstRoot[index], filename);

	if(!searchNode)
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;

	if(!error_code)
		extendedPermissions = checkUserPerms(user , searchNode);

	int mode = atoi(modeIn);
	if(mode & 0b11111100)
		error_code=TECNICOFS_ERROR_INVALID_MODE;

	if(!error_code && !(extendedPermissions&ESPACO_AVAILABLE))
		error_code = TECNICOFS_ERROR_MAXED_OPEN_FILES;
		
	if(!error_code){
		switch(mode){
			case READ:
				if(!(extendedPermissions & USER_CAN_READ && !(extendedPermissions & OPEN_OTHER_WRITE)))
					error_code = TECNICOFS_ERROR_PERMISSION_DENIED;
				break;
			case WRITE:
				if(!(extendedPermissions & USER_CAN_WRITE && !(extendedPermissions & OPEN_OTHER_WRITE) && !(extendedPermissions & OPEN_OTHER_READ)))
					error_code = TECNICOFS_ERROR_PERMISSION_DENIED;
				break;
			case RW:
				if(!(extendedPermissions & USER_CAN_READ && extendedPermissions & USER_CAN_WRITE && !(extendedPermissions & OPEN_OTHER_WRITE) && !(extendedPermissions & OPEN_OTHER_READ)))
					error_code = TECNICOFS_ERROR_PERMISSION_DENIED;
				break;
		}
	}
	if(!error_code){
		for(int i = 0;i<USER_ABERTOS;i++){
			if(user->abertos[i] == -1){
				user->abertos[i] = searchNode->inumber;
				user->mode[i] = mode;
				break;
			}
		}
	}
	sync_unlock(&(fs->bstLock[index]));
	return error_code;
}



int closeFile(tecnicofs *fs, char* filename, client* user){return 0;}
int writeToFile(tecnicofs *fs, char* filename, char* dataInBuffer, client* user){return 0;}
int readFromFile(tecnicofs *fs, char* filename, char* len, client* user){return 0;}


void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	for (int index = 0; index < numberBuckets; index++){
		print_tree(fp, fs->bstRoot[index]);
	}
}

/**
 * HOW TO USE :
 * checkUserPerms(cliente, ficheiro[node]) & CENA_A_TESTAR
 * 
 * user can write					0b00000001
 * user can read					0b00000010
 * file open for writtint by user	0b00000100
 * file open for reading by user	0b00001000
 * file open for writing by other	0b00010000
 * file open for reading by other	0b00100000
 * space to open more files			0b01000000
 */
int checkUserPerms(client* cliente , node* ficheiro){
	uid_t self = cliente->uid;
	uid_t owner;
	permission ownerPerm, othersPerm;
	char* fileContents=NULL;
	int inumber = ficheiro->inumber;
	printf("O inumber e: %d\n", inumber);
	int res=0b00000000, aux = 0b00000000;

	aux = inode_get(ficheiro->inumber,&owner,&ownerPerm,&othersPerm,fileContents,0);
	if(aux<0)
		return TECNICOFS_ERROR_OTHER;
	if(cliente->uid==owner){		//user is owner
		if(ownerPerm&USER_CAN_READ)
			res |= USER_CAN_READ;
		if(ownerPerm&USER_CAN_WRITE)
			res |= USER_CAN_WRITE;
	}
	else{							//user isn't owner
		if(othersPerm&USER_CAN_READ)
			res |= USER_CAN_READ;
		if(othersPerm&USER_CAN_WRITE)
			res |= USER_CAN_WRITE;
	}
	for(int i = 0;i<USER_ABERTOS;i++){
		if(cliente->abertos[i] == -1){
			res|=ESPACO_AVAILABLE;
			break;
		}
	}
	for(int i=0;clients[i]!=NULL && i<MAX_CLIENTS;i++){
		for(int k = 0;k<USER_ABERTOS;k++){
			if(clients[i]->abertos[k]==ficheiro->inumber){
				switch(clients[i]->mode[k]){
					case WRITE:
						if(self == clients[i]->uid)
							res|=OPEN_USER_WRITE;
						else
							res|=OPEN_OTHER_WRITE;
						break;
					case READ:
						if(self == clients[i]->uid)
							res|=OPEN_USER_READ;
						else
							res|=OPEN_OTHER_READ;
						break;
					case RW:
						if(self == clients[i]->uid){
							res|=OPEN_USER_READ;
							res|=OPEN_USER_WRITE;
						}
						else {
							res|=OPEN_OTHER_READ;
							res|=OPEN_OTHER_WRITE;
						}
						break;
				}
				continue;	//verifica user seguinte
			}
		}
	}
	return res;
}
