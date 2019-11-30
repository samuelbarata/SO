#include "fs.h"
#include "lib/safe.h"
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
	if(inumber<0){
		free(perms);
		return TECNICOFS_ERROR_OTHER;
	}

	//insert bst
	fs->bstRoot[index] = insert(fs->bstRoot[index], name, inumber);
	sync_unlock(&(fs->bstLock[index]));
	
	free(perms);
	return 0;
}

int delete(tecnicofs* fs, char *name, client *user){
	int index = hash(name, numberBuckets);
	int error_code = 0, checker=0;
	sync_wrlock(&(fs->bstLock[index]));
	node* searchNode = search(fs->bstRoot[index], name);
	if(!searchNode){
		sync_unlock(&(fs->bstLock[index]));
		return  TECNICOFS_ERROR_FILE_NOT_FOUND;
	}
	
	checker = checkUserPerms(user, searchNode->inumber, TRUE ,NULL,0);
	if(checker<0){
		sync_unlock(&(fs->bstLock[index]));
		return checker;
	}

	if(!(checker&USER_IS_OWNER)){
		sync_unlock(&(fs->bstLock[index]));
		return TECNICOFS_ERROR_PERMISSION_DENIED;
	}

	/*
	if(checker & OPEN_USER){		//fecha o ficheiro se estiver aberto pelo utilizador
		int i;
		char tmp[2]="";
		for(i = 0; i<MAX_OPEN_FILES; i++)
			if(user->ficheiros[i].fd == searchNode->inumber)
				break;
		sprintf(tmp,"%d",i);
		closeFile(fs,tmp,user);
	}*/

	if(!(checker & OPEN_ANY))
		inode_delete(searchNode->inumber);
	
	fs->bstRoot[index] = remove_item(fs->bstRoot[index], name);
	sync_unlock(&(fs->bstLock[index]));
	return error_code;
}

int reName(tecnicofs* fs, char *name, char *newName, client* user){
	int index0 = hash(name, numberBuckets);
	int index1 = hash(newName, numberBuckets);
	int error_code=0, extendedPermissions, inumber=-1;
	sync_rdlock(&(fs->bstLock[index0]));
	node* searchNode = search(fs->bstRoot[index0], name);
	
	if(!searchNode)
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;

	if(!error_code)
		extendedPermissions = checkUserPerms(user , searchNode->inumber,0,NULL,0);

	sync_unlock(&(fs->bstLock[index0]));
	if(!(extendedPermissions & WRITE) && !error_code)
		error_code = TECNICOFS_ERROR_PERMISSION_DENIED;

	if(error_code)
		return error_code;
	
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

	if(!searchNode){
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;
		sync_unlock(&(fs->bstLock[index1]));
		if(index0!=index1)
			sync_unlock(&(fs->bstLock[index0]));
		return error_code;
	}
	inumber = searchNode->inumber;
	fs->bstRoot[index0] = remove_item(fs->bstRoot[index0], name);			//remove
	fs->bstRoot[index1] = insert(fs->bstRoot[index1], newName, inumber);	//adiciona
	
	sync_unlock(&(fs->bstLock[index1]));
	if(index0!=index1)
		sync_unlock(&(fs->bstLock[index0]));
	return error_code;
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
	int checker;
	sync_wrlock(&(fs->bstLock[index]));
	node* searchNode = search(fs->bstRoot[index], filename);

	if(!searchNode)
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;

	if(!error_code){
		checker = checkUserPerms(user , searchNode->inumber,0,NULL,0);
		if(checker<0)
			error_code=checker;
	}

	int mode = atoi(modeIn);
	if((mode & ~RW)&& !error_code)
		error_code=TECNICOFS_ERROR_INVALID_MODE;

	if(error_code){
		sync_unlock(&(fs->bstLock[index]));
		return error_code;
	}

	if(!(checker&ESPACO_AVAILABLE))
		error_code = TECNICOFS_ERROR_MAXED_OPEN_FILES;
		
	if(!error_code && !(checker & mode))
		error_code = TECNICOFS_ERROR_PERMISSION_DENIED;
	
	if(!error_code){
		sync_wrlock(&user->lock);
		for(int i = 0;i<MAX_OPEN_FILES;i++){
			if(user->ficheiros[i].fd == -1){
				user->ficheiros[i].fd = searchNode->inumber;
				user->ficheiros[i].mode = mode;
				user->ficheiros[i].key = safe_strdup(searchNode->key, THREAD);
				error_code = i;
				break;
			}
		}
		sync_unlock(&user->lock);
	}
	sync_unlock(&(fs->bstLock[index]));
	return error_code;
}

int closeFile(tecnicofs *fs, char* fdstr, client* user){
	int fd = atoi(fdstr);
	if(fd<0 || fd>=MAX_OPEN_FILES)
		return TECNICOFS_ERROR_OTHER;
	
	int aux = checkUserPerms(user, user->ficheiros[fd].fd, TRUE, NULL, 0);
	sync_wrlock(&user->lock);
	if(user->ficheiros[fd].fd==FILE_CLOSED){
		sync_unlock(&user->lock);
		return TECNICOFS_ERROR_FILE_NOT_OPEN;
	}

	fd = ficheiroApagadoChecker(fs, user,fd , aux);
	if(fd == FILE_CLOSED){
		sync_unlock(&user->lock);
		return TECNICOFS_ERROR_FILE_NOT_FOUND;
	}
	user->ficheiros[fd].fd = FILE_CLOSED;
	user->ficheiros[fd].mode = NONE;
	free(user->ficheiros[fd].key);
	user->ficheiros[fd].key=NULL;
	sync_unlock(&user->lock);
	return 0;
}

int writeToFile(tecnicofs *fs, char* fdstr, char* dataInBuffer, client* user){
	int fd = atoi(fdstr);

	if(fd<0 || fd>=MAX_OPEN_FILES)
		return TECNICOFS_ERROR_OTHER;
	
	int aux = checkUserPerms(user, user->ficheiros[fd].fd, TRUE, NULL, 0);
	
	sync_wrlock(&user->lock);
	if(user->ficheiros[fd].fd==FILE_CLOSED){
		sync_unlock(&user->lock);
		return TECNICOFS_ERROR_FILE_NOT_OPEN;
	}
	if(!(user->ficheiros[fd].mode & WRITE)){
		sync_unlock(&user->lock);
		return TECNICOFS_ERROR_PERMISSION_DENIED;
	}

	fd = ficheiroApagadoChecker(fs, user,fd , aux);
	if(fd == FILE_CLOSED){
		sync_unlock(&user->lock);
		return TECNICOFS_ERROR_FILE_NOT_FOUND;
	}
	inode_set(user->ficheiros[fd].fd, dataInBuffer, strlen(dataInBuffer));
	sync_unlock(&user->lock);
	return 0;
}

char* readFromFile(tecnicofs *fs, char* fdstr, char* len, client* user){
	int fd = atoi(fdstr), error_code = 0;
	int cmp = atoi(len);
	char *fileContents=malloc(cmp), *ret;
	bzero(fileContents, cmp);
	int aux = checkUserPerms(user, user->ficheiros[fd].fd, TRUE, fileContents, cmp-1);

	if(user->ficheiros[fd].fd==FILE_CLOSED){
		error_code = TECNICOFS_ERROR_FILE_NOT_OPEN;
	}
	if(!error_code&& aux<0){
		error_code = TECNICOFS_ERROR_OTHER;
	}
	sync_wrlock(&user->lock);
	if(!error_code){	
		fd = ficheiroApagadoChecker(fs, user,fd , aux);
	}

	if(!error_code&& fd==FILE_CLOSED){
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;
	}
	if(!error_code&& !(user->ficheiros[fd].mode & READ)){
		error_code = TECNICOFS_ERROR_INVALID_MODE;
	}
	sync_unlock(&user->lock);

	if(error_code){
		sprintf(fileContents, "%d", error_code);
		return fileContents;
	}
	ret = safe_malloc(CODE_SIZE+aux, THREAD);
	sprintf(ret, "%d %s", error_code, fileContents);
	return ret;
}


void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	for (int index = 0; index < numberBuckets; index++){
		print_tree(fp, fs->bstRoot[index]);
	}
}


/**
 * recebe permições XX
 * devolve array [perm1, perm2]
 */
permission *permConv(char* perms){
    int atoiPerms = atoi(perms);
    permission *res = safe_malloc(sizeof(permission)*2, THREAD);
    res[0] = atoiPerms/10;
    res[1] = atoiPerms%10;
    for(int perm=0; perm<2 ;perm++){
        if(res[perm] & ~RW){ //permission is not valid
            res[perm] = TECNICOFS_ERROR_OTHER;
        }
    }
    return res;
}

/**
 * HOW TO USE :
 * checkUserPerms(cliente, ficheiro, 0, NULL, 0) & CENA_A_TESTAR
 * 
 * user can write					0b00000001
 * user can read					0b00000010
 * file open for writtint by user	0b00000100
 * file open for reading by user	0b00001000
 * file open for writing by other	0b00010000
 * file open for reading by other	0b00100000
 * space to open more files			0b01000000
 */
int checkUserPerms(client* cliente , int inumber, int advanced ,char* fileContent, int len){
	uid_t self = cliente->uid;
	uid_t owner;
	permission ownerPerm, othersPerm;
	int res=0b00000000, aux = 0b00000000;
	if(!fileContent)
		len=0;
	if(inumber<0)
		return TECNICOFS_ERROR_OTHER;
	aux = inode_get(inumber,&owner,&ownerPerm,&othersPerm,fileContent,len);
	if(aux<0)
		return TECNICOFS_ERROR_OTHER;
	
	sync_rdlock(&cliente->lock);
	if(cliente->uid==owner){		//user is owner
		res|=USER_IS_OWNER;
		if(ownerPerm&READ)
			res |= READ;
		if(ownerPerm&WRITE)
			res |= WRITE;
	}
	else{							//user isn't owner
		if(othersPerm&READ)
			res |= READ;
		if(othersPerm&WRITE)
			res |= WRITE;
	}
	for(int i = 0;i<MAX_OPEN_FILES;i++){
		if(cliente->ficheiros[i].fd == FILE_CLOSED){
			res|=ESPACO_AVAILABLE;
		}
		else if(cliente->ficheiros[i].fd == inumber){
			if(cliente->ficheiros[i].mode & READ)
				res|=OPEN_USER_READ;
			if(cliente->ficheiros[i].mode & WRITE)
				res|=OPEN_USER_WRITE;
		}
	}
	sync_unlock(&cliente->lock);
	if(advanced)	//verifica quem tem o ficheiro aberto
		for(int i=0;clients[i]!=NULL && i<MAX_CLIENTS;i++){
			sync_rdlock(&clients[i]->lock);
			for(int k = 0;k<MAX_OPEN_FILES;k++){
				if(clients[i]->ficheiros[k].fd==inumber){
					switch(clients[i]->ficheiros[k].mode){
						case NONE:
							break;
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
							if(self == clients[i]->uid)
								res|=OPEN_USER;
							else
								res|=OPEN_OTHER;
							break;
					}
				}
			}
			sync_unlock(&clients[i]->lock);
		}
	return res;
}

/*!!!! CHAMAR FUNCAO DENTRO DE USER WR LOCKS
	recebe cliente e filedescriptor; se ficheiro original inexistente
	fecha ficheiro e remove inode se possivel
	devolve file descriptor do ficheiro
*/
int ficheiroApagadoChecker(tecnicofs *fs, client *user, int fd, int checker){
	//verificar se ficheiro ainda existe ou se foi apagado/renomeado:
	if(user->ficheiros[fd].fd==FILE_CLOSED)
		return FILE_CLOSED;
	int index = hash(user->ficheiros[fd].key, numberBuckets);
	sync_rdlock(&(fs->bstLock[index]));
	node* searchNode = search(fs->bstRoot[index], user->ficheiros[fd].key);
	sync_unlock(&(fs->bstLock[index]));
	if(!searchNode || searchNode->inumber!=user->ficheiros[fd].fd){	//ficheiro original ja nao existe
		if(checker<0)
			return TECNICOFS_ERROR_OTHER;

		if(!(checker&OPEN_OTHER))						//se ficheiro nao estiver aberto por outros utilizadores; inode pode ser apagado
			inode_delete(user->ficheiros[fd].fd);
		
		user->ficheiros[fd].fd = FILE_CLOSED;
		user->ficheiros[fd].mode = NONE;
		free(user->ficheiros[fd].key);
		return FILE_CLOSED;
	}
	return fd;
}