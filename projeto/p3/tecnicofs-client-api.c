#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "tecnicofs-client-api.h"



int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){

}

int tfsDelete(char *filename){

}

int tfsRename(char *filenameOld, char *filenameNew){

}

int tfsOpen(char *filename, permission mode){
	
}

int tfsClose(int fd){

}

int tfsRead(int fd, char *buffer, int len){

}

int tfsWrite(int fd, char *buffer, int len){

}

int tfsMount(char * address){
	int sockfd, servlen;
	struct sockaddr_un serv_addr;


	/* Cria socket stream */
	if ((sockfd= socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		err_dump("client: can't open stream socket");
	


}

int tfsUnmount(){

}
