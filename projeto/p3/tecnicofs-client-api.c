#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "tecnicofs-client-api.h"
#include "globals.h"

int sockfd=-1;
int sendMsg(char* msg);	//return the error code from the server

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
	char* msg;
	int res;
	msg = malloc(sizeof(char)*(strlen(filename)+6));
	sprintf(msg, "%c %s %d%d", 'c',filename, ownerPermissions, othersPermissions);
	res = sendMsg(msg);
	printf("\ncreate: %s, %d\n", msg, res);
	free(msg);
	return res;
}

int tfsDelete(char *filename){
	char* msg;
	int res;
	msg = malloc(sizeof(char)*(strlen(filename)+3));
	sprintf(msg, "%c %s", 'd',filename);
	res = sendMsg(msg);
	printf("\ndelete: %s, %d\n",msg, res);
	free(msg);
	return res;
}

int tfsRename(char *filenameOld, char *filenameNew){
	char* msg;
	int res;
	msg = malloc(sizeof(char)*(strlen(filenameOld)+strlen(filenameNew)+4));
	sprintf(msg, "%c %s %s", 'r',filenameOld, filenameNew);
	res = sendMsg(msg);
	printf("rename: %s, %d\n",msg, res);
	free(msg);
	return res;
}

int tfsOpen(char *filename, permission mode){
	
	return EXIT_SUCCESS;
}

int tfsClose(int fd){
	return EXIT_SUCCESS;
}

int tfsRead(int fd, char *buffer, int len){
	return EXIT_SUCCESS;
}

int tfsWrite(int fd, char *buffer, int len){
	return EXIT_SUCCESS;
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

int sendMsg(char* msg){
	int n;
	char recvline[MAX_INPUT_SIZE];
	
	/*Envia string para sockfd; \0 não é enviado*/
	n=strlen(msg);
	if (write(sockfd, msg, n) != n)
		return TECNICOFS_ERROR_OTHER;

	/* Tenta ler string de sockfd.*/
	n = read(sockfd, recvline, MAX_INPUT_SIZE);
	if (n<0)
		return TECNICOFS_ERROR_OTHER;
	recvline[n]='\0';
	return atoi(recvline);
}