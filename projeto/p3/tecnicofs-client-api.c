#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "tecnicofs-client-api.h"
#include "globals.h"

int sockfd=-1;

int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
	char* msg;
	int res;
	msg = malloc(sizeof(char)*(strlen(filename)+6));
	msg[0] = 'c';
	msg[1] = ' ';
	strcat(msg, filename);
	msg[strlen(filename)+2]=' ';
	msg[strlen(filename)+3]=ownerPermissions + '0';
	msg[strlen(filename)+4]=othersPermissions + '0';
	msg[strlen(filename)+5]='\0';
	res = sendMsg(msg);
	free(msg);
	printf("%d\n", res);
	return res;
}

int tfsDelete(char *filename){
	return EXIT_SUCCESS;
}

int tfsRename(char *filenameOld, char *filenameNew){
	return EXIT_SUCCESS;
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
	
	/*Envia string para sockfd.
	Note-se que o \0 não é enviado*/
	n=strlen(msg);
	if (write(sockfd, msg, n) != n)
		return TECNICOFS_ERROR_OTHER;
	/* Tenta ler string de sockfd.
	Note-se que tem de terminar a string com \0 */
	
	n = read(sockfd, recvline, MAX_INPUT_SIZE);
	if (n<0)
		return TECNICOFS_ERROR_OTHER;
	recvline[n]='\0';

	return atoi(recvline);
}