#include "../unix.h"
#include <stdio.h>
#define MAXLINE 512

main(void){
	int sockfd, servlen;
	struct sockaddr_un serv_addr;


	/* Criasocket stream */
	if ((sockfd= socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		err_dump("client: can't open stream socket");

	/* Primeiroumalimpezapreventiva*/
	bzero((char *) &serv_addr, sizeof(serv_addr));
	/* Dados parao socket stream: tipo+ nomequeidentificao servidor*/
	serv_addr.sun_family= AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen= strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

	/*Estabeleceumaligação.Sófuncionaseosockettiversidocriadoeonomeassociado*/
	if(connect(sockfd,(struct sockaddr*)&serv_addr,servlen)<0)
		err_dump("client:can'tconnecttoserver");
	
	/*Enviaaslinhaslidasdotecladoparaosocket*/
	str_cli(stdin, sockfd);
	/*Fechaosocketetermina*/
	close(sockfd);
	exit(0);
}

/*Lê string de fp e envia para sockfd. Lê string de sockfd e envia para stdout*/
str_cli(fp, sockfd){
	FILE *fp;
	int sockfd;
	int n;
	char sendline[MAXLINE], recvline[MAXLINE+1];
	while(fgets(sendline, MAXLINE, fp)!= NULL) {
		/*Enviastringparasockfd.Note-sequeo\0nãoéenviado*/
		n=strlen(sendline);
		if (write(sockfd, sendline, n) != n)
			err_dump("str_cli:writeerror on socket");
		/* Tentalerstring de sockfd.Note-se quetem de terminara string com \0 */
		n = readline(sockfd, recvline, MAXLINE);
		if (n<0)
			err_dump("str_cli:readlineerror");
		recvline[n]=0;
		/*Enviaastringparastdout*/
		fputs(recvline,stdout);
	}
	if(ferror(fp))
		err_dump("str_cli:errorreadingfile");
}
