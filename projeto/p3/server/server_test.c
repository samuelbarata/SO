#include "../unix.h"
#define MAXLINE 512

main(void){
	int sockfd, newsockfd, clilen, childpid, servlen;
	struct sockaddr_un cli_addr, serv_addr;


	/* Cria socket stream */
	if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0)
		err_dump("server: can't open stream socket");
	
	/*Elimina o nome, para o caso de já existir.*/
	unlink(UNIXSTR_PATH);
	/* O nome serve para que os clientes possam identificar o servidor*/
	
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen= strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	if (bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
		err_dump("server, can't bind local address");
	
	listen(sockfd, 5);

	for(;;){
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
		if(newsockfd < 0)
			err_dump("server:accepterror");
		
		/*Lança processo filho para tratar do cliente*/
		if((childpid = fork()) < 0)
			err_dump("server:forkerror");
		else if(childpid==0){
			/*Processo filho. Fecha sockfd já que não é utilizado pelo processo filho
			Os dados recebidos do cliente são reenviados para o cliente*/
			close(sockfd);
			str_echo(newsockfd);
			exit(0);
		}
		/*Processo pai. Fecha newsockfd que não utiliza*/
		close(newsockfd);
	}
}

/* Servidor do tipo socket stream. Reenviaas linhas recebidas para o cliente*/
str_echo(int sockfd){
	int n;
	char line[MAXLINE];
	for (;;) {/* Lê uma linha do socket */
		n = readline(sockfd, line, MAXLINE);
		if (n == 0)
			return;
		else if (n < 0)
			err_dump("str_echo: readlineerror");
		
		/*Reenvia a linha para o socket. n conta com o \0 da string, caso contrário perdia-se sempre um caracter!*/
		if(write(sockfd,line,n)!=n)
			err_dump("str_echo:writeerror");
	}	
}
