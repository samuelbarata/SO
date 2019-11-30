#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "tecnicofs-client-api.h"
#include "../globals.h"
#include "../lib/safe.h"

int sockfd=-1;
int sendMsg(char* msg, char* res, int len);

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filename)+6), MAIN);
	sprintf(msg, "%c %s %d%d", 'c',filename, ownerPermissions, othersPermissions);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsDelete(char *filename){
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filename)+3), MAIN);
	sprintf(msg, "%c %s", 'd',filename);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsRename(char *filenameOld, char *filenameNew){
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filenameOld)+strlen(filenameNew)+4), MAIN);
	sprintf(msg, "%c %s %s", 'r',filenameOld, filenameNew);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsOpen(char *filename, permission mode){
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(strlen(filename)+5),MAIN);
	sprintf(msg, "%c %s %d", 'o',filename, mode);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsClose(int fd){
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(4), MAIN);
	sprintf(msg, "%c %d", 'x',fd);
	res = sendMsg(msg, NULL,0);
	free(msg);
	return res;
}

int tfsRead(int fd, char *buffer, int len){
	char *msg, *output;
	int res;

	msg = safe_malloc(sizeof(char)*(9),MAIN);	//max len = 9999
	sprintf(msg, "%c %d %d", 'l',fd, len);
	
	output = safe_malloc(len, MAIN);
	res = sendMsg(msg, output, len);
	free(msg);	
	if(res != 0){
		free(output);
		return res;
	}
	res = strlen(output);
	strncpy(buffer, output, res);
	
	free(output);
	return res;
}

int tfsWrite(int fd, char *buffer, int len){
	char* msg;
	int res;
	msg = safe_malloc(sizeof(char)*(6), MAIN);
	sprintf(msg, "%c %d %s", 'w', fd, buffer);
	res = sendMsg(msg, NULL, 0);
	free(msg);
	return res;
}

int tfsMount(char * address){
	// para o cliente ligar ao server
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

	return EXIT_SUCCESS;

}

int tfsUnmount(){	
	int res = close(sockfd);
	sockfd=-1;
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
	if(err!=n)
		return TECNICOFS_ERROR_OTHER;
	
	debug_print("%s ", msg);

	/* Tenta ler string de sockfd.*/
	if(res)
		len+=MAX_INPUT_SIZE;
	else
		len=MAX_INPUT_SIZE;

	recvline = safe_malloc(len, MAIN);
	bzero(recvline, MAX_INPUT_SIZE);
	n = read(sockfd, recvline, MAX_INPUT_SIZE);
	if (n<0)
		return TECNICOFS_ERROR_OTHER;
	debug_print("%s\n", recvline);
	sscanf(recvline, "%d %s", &n, res);
	free(recvline);
	return n;
}