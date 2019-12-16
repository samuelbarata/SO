#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "tecnicofs-client-api.h"

#define FILE_CLOSED		-1
#define MAX_INPUT_SIZE	1024
#ifdef DEBUG
	#define DEBUG_TEST	1
#else
	#define DEBUG_TEST	0
#endif

/*Prints debug information to stdout
Uses similar syntax to fprintf*/
#define debug_print(...) do { if (DEBUG_TEST) fprintf(stdout, __VA_ARGS__);fflush(stdout);} while (0)
void *safe_malloc(size_t __size){
	void* p;
	p = malloc(__size);
	if(!p){
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}
	return p;
}

int sockfd=FILE_CLOSED;
int sendMsg(char* msg, char* res, int len);

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
	if(sockfd==FILE_CLOSED)
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filename)+6));
	sprintf(msg, "%c %s %d%d", 'c',filename, ownerPermissions, othersPermissions);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsDelete(char *filename){
	if(sockfd==FILE_CLOSED)
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filename)+3));
	sprintf(msg, "%c %s", 'd',filename);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsRename(char *filenameOld, char *filenameNew){
	if(sockfd==FILE_CLOSED)
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filenameOld)+strlen(filenameNew)+4));
	sprintf(msg, "%c %s %s", 'r',filenameOld, filenameNew);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsOpen(char *filename, permission mode){
	if(sockfd==FILE_CLOSED)
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filename)+5));
	sprintf(msg, "%c %s %d", 'o',filename, mode);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsClose(int fd){
	if(sockfd==FILE_CLOSED)
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(4));
	sprintf(msg, "%c %d", 'x',fd);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsRead(int fd, char *buffer, int len){
	if(sockfd==FILE_CLOSED)
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	char *msg, *output;
	int res;

	msg = safe_malloc(sizeof(char)*(9));	//max len = 9999
	sprintf(msg, "%c %d %d", 'l',fd, len);
	
	output = safe_malloc(len);
	bzero(output, len);
	res = sendMsg(msg, output, len);

	free(msg);	
	if(res <= 0){
		free(output);
		return res;
	}
	if(buffer && res>=2)
		strncpy(buffer, output, res);
	
	free(output);
	return res;
}

int tfsWrite(int fd, char *buffer, int len){
	if(sockfd==FILE_CLOSED)
		return TECNICOFS_ERROR_NO_OPEN_SESSION;
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(6));
	sprintf(msg, "%c %d %s", 'w', fd, buffer);
	res = sendMsg(msg, NULL, 0);
	free(msg);
	return res;
}

int tfsMount(char * address){
	if(sockfd>=0)
		return TECNICOFS_ERROR_OPEN_SESSION;
	int servlen;
	struct sockaddr_un serv_addr;

	/* Cria socket stream */
	if ((sockfd= socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return TECNICOFS_ERROR_CONNECTION_ERROR;

	/* limpar o address */
	bzero((char*) &serv_addr, sizeof(serv_addr));

	/* valores para o serv_addr */
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, address);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

	/* liga este socket ao server */
	if(connect(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
		return TECNICOFS_ERROR_CONNECTION_ERROR;

	return 0;

}

int tfsUnmount(){	
	int res = close(sockfd);
	sockfd=FILE_CLOSED;
	return res;
}

int sendMsg(char* msg, char* res, int len){
	int n, err;
	char *recvline;
	
	/*Envia string para sockfd; \0 não é enviado*/
	n=strlen(msg);
	err = write(sockfd, msg, n);
	if(err<0)
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	if(err!=n){
		perror("write");
		return TECNICOFS_ERROR_OTHER;
	}
	debug_print("%s", msg);

	/* Tenta ler string de sockfd.*/
	if(res)
		len+=MAX_INPUT_SIZE;
	else
		len=MAX_INPUT_SIZE;

	recvline = safe_malloc(len);
	bzero(recvline, len);
	n = read(sockfd, recvline, len);
	debug_print("\t\t%s\n", recvline);
	if(n==0)
		return TECNICOFS_ERROR_CONNECTION_ERROR;
	else if (n<0){
		perror("read");
		return TECNICOFS_ERROR_OTHER;
	}
	
	sscanf(recvline, "%d %s", &n, res);

	free(recvline);
	return n;
}