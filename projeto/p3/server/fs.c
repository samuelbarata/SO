#include "fs.h"
#include "../lib/safe.h"
#include "../lib/bst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../lib/sync.h"
#include "../lib/hash.h"
#include "../lib/inodes.h"


tecnicofs* new_tecnicofs(){
	tecnicofs*fs = safe_malloc(sizeof(tecnicofs), MAIN);

	fs->bstRoot = safe_malloc(sizeof(node*)*numberBuckets, MAIN);
	fs->bstLock = safe_malloc(sizeof(syncMech)*numberBuckets, MAIN);

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


/**
 * cria um ficheiro no fs
*/
int create(tecnicofs* fs, char *name,client *user ,permission *perms){
	int index = hash(name, numberBuckets);
	sync_wrlock(&(fs->bstLock[index]));
	int inumber = lookup(fs, name);

	if(inumber != TECNICOFS_ERROR_FILE_NOT_FOUND){
		sync_unlock(&(fs->bstLock[index]));
		free(perms);
		return TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
	}
	if((perms[0] | perms[1]) & ~RW){
		sync_unlock(&(fs->bstLock[index]));
		free(perms);
		return TECNICOFS_ERROR_INVALID_PERMISSION;
	}

	inumber = inode_create(user->uid, perms[0], perms[1]);
	free(perms);

	if(inumber<0){
		sync_unlock(&(fs->bstLock[index]));
		return TECNICOFS_ERROR_MAXED_FILES;
	}

	fs->bstRoot[index] = insert(fs->bstRoot[index], name, inumber);
	sync_unlock(&(fs->bstLock[index]));
	return 0;
}

/**
 * Apaga um ficheiro do fs
 * Se o ficherio estiver aberto pelo utilizador; é fechado
 * inode apenas é apagado quando mais ninguem tiver o ficheiro aberto
*/
int delete(tecnicofs* fs, char *name, client *user){
	int index = hash(name, numberBuckets);
	int error_code = 0, aux=0;
	sync_wrlock(&(fs->bstLock[index]));
	int inumber = lookup(fs, name);

	if(inumber == TECNICOFS_ERROR_FILE_NOT_FOUND){
		sync_unlock(&(fs->bstLock[index]));
		return TECNICOFS_ERROR_FILE_NOT_FOUND;
	}

	aux = checkUserPerms(user, inumber ,NULL,0);
	if(aux<0){
		sync_unlock(&(fs->bstLock[index]));
		return aux;
	}

	if(!(aux&USER_IS_OWNER)){
		sync_unlock(&(fs->bstLock[index]));
		return TECNICOFS_ERROR_PERMISSION_DENIED;
	}

	if(aux & OPEN_USER){		//fecha o ficheiro se estiver aberto pelo utilizador
		int i;
		for(i = 0; i<MAX_OPEN_FILES; i++)
			if(user->ficheiros[i].inumber == inumber)
				break;
		free_file(user, i);
	}
	inode_delete(inumber);
	
	fs->bstRoot[index] = remove_item(fs->bstRoot[index], name);
	sync_unlock(&(fs->bstLock[index]));
	return error_code;
}

/**
 * Muda o nome de um ficheiro
 * Se o ficheiro estiver aberto pelo utilizador; o nome é trocado também
 * Para outros utilizadores com o ficheiro aberto -> FILE_NOT_FOUND
 */
int reName(tecnicofs* fs, char *name, char *newName, client* user){
	int index0 = hash(name, numberBuckets);
	int index1 = hash(newName, numberBuckets);
	int error_code=0, aux;
	sync_rdlock(&(fs->bstLock[index0]));
	int inumber0 = lookup(fs, name);
	sync_unlock(&(fs->bstLock[index0]));
	sync_rdlock(&(fs->bstLock[index1]));
	int inumber1 = lookup(fs, newName);
	sync_unlock(&(fs->bstLock[index1]));

	if(inumber0 < 0)
		return inumber0;	//file not found
	
	if(inumber1>=0)
		return TECNICOFS_ERROR_FILE_ALREADY_EXISTS;


	aux = checkUserPerms(user , inumber0,NULL,0);

	if(!(aux & USER_IS_OWNER))
		return TECNICOFS_ERROR_PERMISSION_DENIED;

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

	inumber0 = lookup(fs, name);
	inumber1 = lookup(fs, newName);
	if(inumber0 < 0)
		error_code = inumber0;	//file not found
	else if(inumber1>=0)
		error_code = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
	if(error_code){
		sync_unlock(&(fs->bstLock[index1]));
		if(index0!=index1)
			sync_unlock(&(fs->bstLock[index0]));
		return error_code;
	}

	//se o cliente tiver o ficheiro aberto; também é renomeado
	if(aux & OPEN_USER){
		int i;
		for(i = 0;user->ficheiros[i].inumber!=inumber0;i++);
		free(user->ficheiros[i].filename);
		user->ficheiros[i].filename=safe_strdup(newName, THREAD);
	}

	fs->bstRoot[index0] = remove_item(fs->bstRoot[index0], name);			//remove
	fs->bstRoot[index1] = insert(fs->bstRoot[index1], newName, inumber0);	//adiciona
	
	sync_unlock(&(fs->bstLock[index1]));
	if(index0!=index1)
		sync_unlock(&(fs->bstLock[index0]));
	return error_code;
}

/**
 * se o ficheiro já está abero; acrescenta as permições novas
 * se o ficheiro esta fecahdo, abre
 * 
 * devolve o fd do ficheiro aberto
*/
int openFile(tecnicofs *fs, char* filename,char* modeIn, client* user){
	int index = hash(filename, numberBuckets),i;
	int error_code = 0;
	int aux;
	sync_rdlock(&(fs->bstLock[index]));
	int inumber = lookup(fs, filename);

	if(inumber<0)	//ficheiro existe?
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;
	else{			//vai buscar permições
		aux = checkUserPerms(user ,inumber,NULL,0);
		if(aux<0)
			error_code=aux;
	}
	int mode = atoi(modeIn);
	if((mode & ~RW) && !error_code)	//modo existe?
		error_code=TECNICOFS_ERROR_INVALID_PERMISSION;
	if(mode == 0 && !error_code)	//modo valido?
		error_code = TECNICOFS_ERROR_INVALID_MODE;
	
	if(error_code){
		sync_unlock(&(fs->bstLock[index]));
		return error_code;
	}

	if(aux & OPEN_USER){
		for(i = 0;i<MAX_OPEN_FILES;i++)
			if(user->ficheiros[i].inumber == inumber)
				break;
		user->ficheiros[i].mode = user->ficheiros[i].mode | mode;
		sync_unlock(&(fs->bstLock[index]));
		return i;
	}

	if(!(aux&ESPACO_AVAILABLE))				//espaço para abrir
		error_code = TECNICOFS_ERROR_MAXED_OPEN_FILES;
		
	if(!error_code && !(aux & mode))		//permições pra abrir
		error_code = TECNICOFS_ERROR_PERMISSION_DENIED;
	
	if(!error_code){
		for(i = 0;i<MAX_OPEN_FILES;i++){
			if(user->ficheiros[i].inumber == FILE_CLOSED){
				inode_open(inumber);
				user->ficheiros[i].inumber = inumber;
				user->ficheiros[i].mode = mode;
				user->ficheiros[i].filename = safe_strdup(filename, THREAD);
				error_code = i;
				break;
			}
		}
	}
	sync_unlock(&(fs->bstLock[index]));
	return error_code;
}

int closeFile(tecnicofs *fs, char* fdstr, client* user){
	int fd = atoi(fdstr);
	if(fd<0 || fd>=MAX_OPEN_FILES)
		return TECNICOFS_INVALID_FD;
	
	
	if(user->ficheiros[fd].inumber==FILE_CLOSED){
		return TECNICOFS_ERROR_FILE_NOT_OPEN;
	}
	int aux = checkUserPerms(user, user->ficheiros[fd].inumber, NULL, 0);
	
	if(fileRenamed(fs, user, fd)!=fd)
		return TECNICOFS_ERROR_FILE_RENAMED;
	
	free_file(user, fd);
	if(aux & FILE_DELETED){
		return TECNICOFS_ERROR_FILE_NOT_FOUND;
	}
	return 0;
}

int writeToFile(tecnicofs *fs, char* fdstr, char* dataInBuffer, client* user){
	int fd = atoi(fdstr);
	if(fd<0 || fd>=MAX_OPEN_FILES)
		return TECNICOFS_INVALID_FD;
	
	if(user->ficheiros[fd].inumber==FILE_CLOSED){
		return TECNICOFS_ERROR_FILE_NOT_OPEN;
	}

	int aux = checkUserPerms(user, user->ficheiros[fd].inumber, NULL, 0);
	if(aux & FILE_DELETED){
		free_file(user, fd);
		return TECNICOFS_ERROR_FILE_NOT_FOUND;
	}
	if(fileRenamed(fs, user, fd)!=fd)
		return TECNICOFS_ERROR_FILE_RENAMED;

	if(!(aux & WRITE))
		return TECNICOFS_ERROR_PERMISSION_DENIED;

	if(!(user->ficheiros[fd].mode & WRITE)){
		return TECNICOFS_ERROR_INVALID_MODE;
	}

	inode_set(user->ficheiros[fd].inumber, dataInBuffer, strlen(dataInBuffer));
	return 0;
}

char* readFromFile(tecnicofs *fs, char* fdstr, char* len, client* user){
	int fd = atoi(fdstr), error_code = 0, aux;
	int cmp = atoi(len);
	char *fileContents=NULL, *ret;
	
	if(user->ficheiros[fd].inumber==FILE_CLOSED)
		error_code = TECNICOFS_ERROR_FILE_NOT_OPEN;

	else{
		cmp = cmp<=0 ? 1 : cmp;
		fileContents=safe_malloc(cmp, THREAD);
		bzero(fileContents, cmp);
		aux = checkUserPerms(user, user->ficheiros[fd].inumber, fileContents, cmp-1);
		if(aux<0)
			error_code = aux;
	}
	if(error_code);
	else if(aux & FILE_DELETED){
		free_file(user, fd);
		error_code = TECNICOFS_ERROR_FILE_NOT_FOUND;
	}
	else if(fileRenamed(fs, user, fd)!=fd){
		error_code = TECNICOFS_ERROR_FILE_RENAMED;
	}
	else if(!(aux & READ)){
		error_code = TECNICOFS_ERROR_PERMISSION_DENIED;
	}
	else if(!(user->ficheiros[fd].mode & READ)){
		error_code = TECNICOFS_ERROR_INVALID_MODE;
	}

	if(error_code){
		free(fileContents);
		fileContents = safe_malloc(CODE_SIZE, THREAD);
		bzero(fileContents, CODE_SIZE);
		sprintf(fileContents, "%d", error_code);
		return fileContents;
	}
	ret = safe_malloc(3*CODE_SIZE+cmp, THREAD);
	bzero(ret, 3*CODE_SIZE+cmp);
	error_code=strlen(fileContents);
	sprintf(ret, "%d %s", error_code, fileContents);
	free(fileContents);
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
			res[perm] = TECNICOFS_ERROR_INVALID_PERMISSION;
		}
	}
	return res;
}

/**
 * HOW TO USE :
 * checkUserPerms(cliente, ficheiro, NULL, 0) & CENA_A_TESTAR
 * 
 * user can write					0b00000001
 * user can read					0b00000010
 * file open for writtint by user	0b00000100
 * file open for reading by user	0b00001000
 * space to open more files			0b01000000
 */
int checkUserPerms(client* cliente , int inumber ,char* fileContent, int len){
	uid_t owner;
	permission ownerPerm, othersPerm;
	int res=0b00000000, aux = 0b00000000, deleted;
	if(!fileContent)
		len=0;
	if(inumber<0)
		return TECNICOFS_ERROR_OTHER;
	aux = inode_get(inumber,&owner,&ownerPerm,&othersPerm,fileContent,len,&deleted);
	if(aux<0)
		return TECNICOFS_ERROR_OTHER;
	
	if(deleted)
		res |= FILE_DELETED;

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
		if(cliente->ficheiros[i].inumber == FILE_CLOSED){
			res|=ESPACO_AVAILABLE;
		}
		else if(cliente->ficheiros[i].inumber == inumber){
			if(cliente->ficheiros[i].mode & READ)
				res|=OPEN_USER_READ;
			if(cliente->ficheiros[i].mode & WRITE)
				res|=OPEN_USER_WRITE;
		}
	}
	return res;
}


/**
 * Recebe user e file descriptor
 * Fecha o ficheiro
*/
void free_file(client* user, int fd){
	inode_close(user->ficheiros[fd].inumber);
	user->ficheiros[fd].inumber = FILE_CLOSED;
	user->ficheiros[fd].mode = NONE;
	free(user->ficheiros[fd].filename);
	user->ficheiros[fd].filename=NULL;
}

/**
 * !!DENTRO LOCKS
 * Recebe fs e name
 * devolve inumber correspondente
*/
int lookup(tecnicofs *fs, char *name){
	int index = hash(name, numberBuckets);
	node* searchNode = search(fs->bstRoot[index], name);
	return searchNode ? searchNode->inumber : TECNICOFS_ERROR_FILE_NOT_FOUND;
}

/**
 * Usada para ver se um ficheiro foi renomeado
 * percorre file system procurando um node com inumber do ficheiro anterios
 * se existir o ficheiro foi renomead
 * caso contrario, apagado
 * devolve o node do ficheiro novo; ou NULL
 */
node* nodeLookup(tecnicofs *fs, int inumber){
	for(int i = 0;i<numberBuckets;i++){
		sync_rdlock(&(fs->bstLock[i]));
		node* searchNode = inumberLookup(fs->bstRoot[i], inumber);
		sync_unlock(&(fs->bstLock[i]));
		if(searchNode)
			return searchNode;
	}
	return NULL;
}

int fileRenamed(tecnicofs* fs, client* user, int fd){
	int index=hash(user->ficheiros[fd].filename, numberBuckets);
	sync_rdlock(&(fs->bstLock[index]));
	int inumber = lookup(fs, user->ficheiros[fd].filename);
	sync_unlock(&(fs->bstLock[index]));
	if(inumber != user->ficheiros[fd].inumber){
		free_file(user, fd);
		return TECNICOFS_ERROR_FILE_RENAMED;
	}
	return fd;
}

void free_cliente(client* cliente){
	for(int i = 0; i<MAX_OPEN_FILES; free(cliente->ficheiros[i++].filename));
	free(cliente);
}
