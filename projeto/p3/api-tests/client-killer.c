#define _GNU_SOURCE
#include "../tecnicofs-api-constants.h"
#include "../client/tecnicofs-client-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#define TESTES 4

char testes[TESTES][100]= {"./client-api-test-create /tmp/mySocket", "./client-api-test-delete /tmp/mySocket", "./client-api-test-read /tmp/mySocket", "./client-api-test-success /tmp/mySocket"};

char buffer[100];
char testSocket[32] = "/tmp/testSocket";
int serverSocket; 
int otherSocket; 
int contador=0;
int fd;


void user(char* sock);
void other(char* sock);

void wait(int fd){
	read(fd, buffer, 16);
}
void post(int fd){
	dprintf(fd, "c");
}
void pauser(int fd){
	post(fd);
	printf("pausing %d\n", contador++);
	wait(fd);
}


void init(){
	int sockfd;
	struct sockaddr_un serv_addr;
	int servlen;
	struct sockaddr_un cli_addr;
	int clilen = sizeof(cli_addr);
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	unlink(testSocket);
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, testSocket);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	bind(sockfd, (struct sockaddr*) &serv_addr, servlen);
	listen(sockfd, 5);
	otherSocket = accept(sockfd,(struct sockaddr*) &cli_addr,(socklen_t*) &clilen);
}
void conect(){
	int servlen;
	struct sockaddr_un serv_addr;
	serverSocket= socket(AF_UNIX, SOCK_STREAM, 0);
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, testSocket);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	connect(serverSocket, (struct sockaddr*) &serv_addr, servlen);
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Usage: %s sock_path\n", argv[0]);
		exit(0);
	}
	if(getuid()==1000){
		bzero(buffer, 100);
		for(int i = 0;i<TESTES;i++){
			strncpy(buffer, testes[i], 99);
			system(buffer);
		}
		fprintf(stdout,"Open a terminal and run this file as root while leaving this one open\nsudo %s %s\n", argv[0], argv[1]);
		fflush(stdout);
		init();
		user(argv[1]);
	}
	else{
		conect();
		other(argv[1]);
	}
}


/*
wait(sockfd);
post(sockfd);
pauser(sockfd);
*/


void user(char* sock){
	int sockfd = otherSocket;
	system("whoami");
	assert(tfsMount(sock) == 0);
	assert(tfsCreate("z", RW, RW) == 0);
	pauser(sockfd);
	assert(tfsRename("z", "k")==0);

	assert(tfsUnmount() == 0);
	fprintf(stdout, "SUCCESS\n");
	pauser(sockfd);

	printf("\nteste2\n");
	assert(tfsMount(sock) == 0);
	fd = tfsOpen("abc", RW);
	assert(fd>=0);
	pauser(sockfd);
	assert(tfsWrite(fd, "ola",0)==TECNICOFS_ERROR_FILE_NOT_FOUND);


	assert(tfsUnmount() == 0);
	fprintf(stdout, "SUCCESS\n");

}



void other(char* sock){
	int sockfd = serverSocket;
	system("whoami");
	assert(tfsMount(sock) == 0);
	
	printf("\npausa dramatica bue irritante\n");
	sleep(2);

	wait(sockfd);
	assert(tfsCreate("z", RW, NONE) == TECNICOFS_ERROR_FILE_ALREADY_EXISTS);	//criar ficheiro existente
	assert(tfsOpen("z", RW)==0);												//abrir ficheiro outro user com perms

	pauser(sockfd);

	assert(tfsRead(0, NULL, 0)==TECNICOFS_ERROR_FILE_NOT_FOUND);				//ler ficheiro aberto e apagado
	fd = tfsOpen("k", WRITE);													//abrir ficheiro after rename
	assert(fd>=0);
	assert(tfsRead(fd, NULL, 0)==TECNICOFS_ERROR_INVALID_MODE);					//ler ficheiro modo invalido
	assert(tfsClose(fd)==0);													//fechar ficheiro certo
	fd = tfsOpen("k", WRITE);													//abrir ficheiro after rename
	printf("%d", fd);
	assert(fd>=0);
	assert(tfsRead(fd, buffer, 0)==TECNICOFS_ERROR_INVALID_MODE);					//ler ficheiro modo errado
	assert(tfsUnmount() == 0);
	fprintf(stdout, "SUCCESS\n");
	
	printf("\nteste2\n");
	assert(tfsMount(sock) == 0);
	assert(tfsCreate("abc", RW, RW) == 0);
	
	pauser(sockfd);
	
	fd = tfsOpen("abc", RW);
	assert(fd>=0);
	assert(tfsWrite(fd, "ola", 4)==0);
	assert(tfsRename("abc", "def") == 0);
	assert(tfsCreate("abc", RW, RW) == 0);
	
	post(sockfd);
	assert(tfsRead(fd,buffer, 4)==3);
	assert(tfsUnmount() == 0);

	fprintf(stdout, "SUCCESS\n");
}
