#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include "../lib/globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <limits.h>

#include "../lib/safe.h"
#include "../lib/timer.h"
#include "../lib/inodes.h"
#include "fs.h"
#include "../lib/sync.h"

char* global_SocketName = NULL;
char* global_outputFile = NULL;
int numberBuckets;
int sockfd;						//socket servidor
pthread_t* workers=NULL;		//ligacoes existentes
FILE* outputFp=NULL;			//ficheiro de output do server
sigset_t set;					//sinais a ignorar pelas threads
tecnicofs* fs;					//filesystem
TIMER_T startTime, stopTime;
int nClients;

static void displayUsage (const char* appName){
	fprintf(stderr,"Usage: %s nomesocket output_filepath numbuckets[>=1]\n", appName);
	exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
	if (argc != 4) {
		fprintf(stderr,"Invalid format:\n");
		displayUsage(argv[0]);
	}

	global_SocketName = argv[1];
	global_outputFile = argv[2];
	numberBuckets = atoi(argv[3]);
	if (numberBuckets<=0) {
		fprintf(stderr,"Invalid number of buckets\n");
		displayUsage(argv[0]);
	}
}

FILE * openOutputFile() {
	FILE *fp;
	fp = fopen(global_outputFile, "w");
	if (fp == NULL) {
		perror("Error opening output file");
		exit(EXIT_FAILURE);
	}
	return fp;
}

/**
 * Recebe comando e cliente que executa o pedido
 * Devolve a mensagem a ser enviada para o cliente
*/
char* applyCommand(char* command, client* user){
	char token;
	char arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
	char *res=NULL;
	int code=INT_MAX;

	bzero(arg1, MAX_INPUT_SIZE);
	bzero(arg2, MAX_INPUT_SIZE);

	if (command == NULL)
		return 0;

	if(sscanf(command, "%c %s %s", &token, arg1, arg2)<0){
		perror("error processing input");
		res = safe_malloc(CODE_SIZE, THREAD);
		sprintf(res, "%d", TECNICOFS_ERROR_OTHER);
		return res;
	}

	//verificação erro comandos
	switch(token){
		case 'r':
		case 'l':
		case 'o':
		case 'w':
			if(!strcmp(arg2, "")){
				fprintf(stderr,"Error: could not apply command %c\n", token);
				code= TECNICOFS_ERROR_OTHER;
			}
		case 'c':
		case 'x':
		case 'd':
			if(!strcmp(arg1, "")){
				fprintf(stderr,"Error: could not apply command %c\n", token);
				code= TECNICOFS_ERROR_OTHER;
			}
			break;
		default: { /* error */
			fprintf(stderr,"Error: could not apply command %c\n", token);
			code= TECNICOFS_ERROR_OTHER;
		}
	}

	if(code == INT_MAX)	//n ocorreu erro
		switch (token) {
			case 'c':
				code= create(fs, arg1, user, permConv(arg2));
				break;
			case 'd':
				code= delete(fs, arg1, user);
				break;
			case 'r':
				code= reName(fs, arg1, arg2, user);
				break;
			case 'l':
				//devolve ja a mensagem organizada
				return readFromFile(fs, arg1, arg2, user);
				break;
			case 'o':
				code= openFile(fs, arg1, arg2, user);
				break;
			case 'x':
				code= closeFile(fs, arg1, user);
				break;
			case 'w':
				code= writeToFile(fs, arg1, arg2, user);
				break;
				
			default: { /* error */
				fprintf(stderr,"Error: could not apply command %c\n", token);
				code= TECNICOFS_ERROR_OTHER;
			}
		}

	//grava o codigo para string
	res = safe_malloc(CODE_SIZE, THREAD);
	sprintf(res, "%d", code);
	return res;
}

/**
 * Inicializa o servidor
*/
void inits(){ 
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	workers = safe_malloc(sizeof(pthread_t*)*MAX_CLIENTS, MAIN);
	struct sockaddr_un serv_addr;
	int servlen;

	srand(time(NULL));

	//Cria socket stream
	sockfd = safe_socket(AF_UNIX, SOCK_STREAM, 0);

	//Elimina o nome, para o caso de já existir.
	unlink(global_SocketName);

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, global_SocketName);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	safe_bind(sockfd, (struct sockaddr*) &serv_addr, servlen);

	listen(sockfd, 5);
	TIMER_READ(startTime);
}


void *newClient(void* cli){
	safe_sigmask(SIG_SETMASK, &set, NULL);

	client *cliente = (client*)cli;
	debug_print("NEW CONNECTION: %02d:%d\n",cliente->socket, cliente->uid);

	int n;
	char *res;
	char line[MAX_INPUT_SIZE];

	while(TRUE){
		bzero(line, MAX_INPUT_SIZE);
		n = recv(cliente->socket, line, MAX_INPUT_SIZE, 0);
		if (n == 0)
			break;
		else if (n < 0){ //Error
			perror("read from socket");
			break;
		}

		//prints debug input from client
		debug_print("%02d:%d: %s",cliente->socket,cliente->uid, line);
		
		res = applyCommand(line, cliente);
		//prints debug output to client
		debug_print(": %s\n",res);

		n = dprintf(cliente->socket, "%s", res);
		free(res);
		if(n < 0){ //Error
			perror("writing to socket");
			break;
		}
	}
	debug_print("EXIT CLIENT: %02d:%d\n",cliente->socket,cliente->uid);
	close(cliente->socket);
	free(cliente);
	return NULL;
}

void connections(){
	unsigned int ucred_len;
	struct ucred ucreds;
	int newsockfd, clilen;
	struct sockaddr_un cli_addr;
	client *cliente;
	clilen = sizeof(cli_addr);
	ucred_len = sizeof(struct ucred);
	for(nClients=0;nClients<MAX_CLIENTS;nClients++){
		newsockfd = safe_accept(sockfd,(struct sockaddr*) &cli_addr,(socklen_t*) &clilen);	//accept client
		cliente = safe_malloc(sizeof(client), THREAD);										//malloc client
		
		if(getsockopt(newsockfd, SOL_SOCKET, SO_PEERCRED, &ucreds, &ucred_len) == -1){		//get user credentials
			perror("cant get client uid");
			close(newsockfd);
			free(cliente);
			nClients--;
			continue;
		}
		cliente->socket=newsockfd;		//save credentials
		cliente->uid=ucreds.uid;

		for(int i = 0; i < MAX_OPEN_FILES; i++){	//init empty file array
			cliente->ficheiros[i].inumber = FILE_CLOSED;
			cliente->ficheiros[i].mode = NONE;
		}

		safe_pthread_create(&workers[nClients], NULL, newClient, (void*)cliente);	//iniciar threas clientes
	}
	fprintf(stderr, "Max client number reached\n");
	raise(SIGINT);
}


/**
 * Terminação do servidor
*/
void exitServer(){
	debug_print("\b\bExitting Server...");
	close(sockfd);		//não deixa receber mais ligações

	for(int i = 0; i<nClients; i++)		//espera que threads acabem os trabalhos dos clientes
		safe_pthread_join(workers[i], NULL, IGNORE);

	TIMER_READ(stopTime);
	print_tecnicofs_tree(outputFp, fs);
	fprintf(outputFp, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, stopTime));
	fflush(outputFp);
	fclose(outputFp);
	free_tecnicofs(fs);
	free(workers);
	debug_print("%sServer Exited.%s\n", "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", "    ");
	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
	signal(SIGINT, exitServer);
	parseArgs(argc, argv);

	outputFp = openOutputFile();
	fs = new_tecnicofs();

	inits();
	connections();

	/*servidor sai com o sinal
	se chegar aqui algo correu mal*/
	exit(EXIT_FAILURE);
}
