#define _GNU_SOURCE
#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <limits.h>

#include "lib/safe.h"
#include "lib/timer.h"
#include "lib/inodes.h"
#include "fs.h"
#include "sync.h"

char* global_SocketName = NULL;
char* global_outputFile = NULL;
int numberBuckets = 0;
int stop = 0;           //variavel global avisa quando todos os comandos foram processados
int sockfd;             //socket servidor
pthread_t* workers=NULL;    //ligacoes existentes
client* clients[MAX_CLIENTS];      //array clients

int numberCommands = 0; //numero de comandos a processar

pthread_mutex_t commandsLock;   //lock do vetor de comandos
sigset_t set;
FILE* outputFp=NULL;

tecnicofs* fs;
TIMER_T startTime, stopTime;



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

char* applyCommands(char* command, client* user){
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
            res= readFromFile(fs, arg1, arg2, user);
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
    if(code == INT_MAX)
        return res;
    
    res = safe_malloc(CODE_SIZE, THREAD);
    sprintf(res, "%d", code);
    return res;
}

void inits(){ 
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
    workers = safe_malloc(sizeof(pthread_t*)*MAX_CLIENTS, MAIN);
    struct sockaddr_un serv_addr;
    int servlen;

    bzero(clients, MAX_CLIENTS);

    srand(time(NULL));

    /* Cria socket stream */
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("server: can't open stream socket");
		exit(EXIT_FAILURE);
	}

    /*Elimina o nome, para o caso de já existir.*/
	unlink(global_SocketName);

    bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, global_SocketName);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	if (bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
		perror("server: can't bind local address");
	
	listen(sockfd, 5);
    TIMER_READ(startTime);
}


void *newClient(void* cli){
	if(pthread_sigmask(SIG_SETMASK, &set, NULL)){
		perror("sig_mask: Failure");
		pthread_exit(NULL);
	}
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
		else if (n < 0){ //connectionError
			perror("read from socket");
			free(cliente);
			pthread_exit(NULL);
		}
		debug_print("%02d:%d: %s",cliente->socket,cliente->uid, line);

		fflush(stdout);
		res = applyCommands(line, cliente);

        debug_print(": %s\n",res);

		n = dprintf(cliente->socket, "%s", res);
		free(res);
		if(n < 0){
			perror("dprintf");
			pthread_exit(NULL);
		}
	}
	debug_print("EXIT CLIENT: %02d:%d\n",cliente->socket,cliente->uid);
    sync_destroy(&cliente->lock);
	free(cliente);
	return NULL;
}

void connections(){
    int nClients=0, err;
    unsigned int ucred_len;

    struct ucred ucreds;

    int newsockfd, clilen;
	struct sockaddr_un cli_addr;
    client *cliente;

    for(;;){
		clilen = sizeof(cli_addr);
		if(nClients>=MAX_CLIENTS){
			fprintf(stdout, "max client number reached");
			raise(SIGINT);
		}
		newsockfd = accept(sockfd,(struct sockaddr*) &cli_addr,(socklen_t*) &clilen);
		if(newsockfd < 0)
			perror("server: accept error");

        cliente = malloc(sizeof(client));
        nClients++;

		ucred_len = sizeof(struct ucred);
        if(getsockopt(newsockfd, SOL_SOCKET, SO_PEERCRED, &ucreds, &ucred_len) == -1){
			perror("cant get client uid");
			free(cliente);
			continue;
		}
        cliente->socket=newsockfd;
        cliente->uid=ucreds.uid;

        for(int i = 0; i < MAX_OPEN_FILES; i++){
            cliente->ficheiros[i].fd = FILE_CLOSED;
            cliente->ficheiros[i].mode = NONE;
            sync_init(&cliente->lock);
            cliente->ficheiros[i].key=NULL;
        }

        
		clients[nClients-1]=cliente;

        err = pthread_create(&workers[nClients-1], NULL, newClient, (void*)cliente);
        if (err){
            perror("Can't create thread");
            exit(EXIT_FAILURE);
        }
	}
}

void exitServer(){
	pthread_sigmask(SIG_SETMASK, &set, NULL);

    int join;
    debug_print("\b\bExitting Server...");
    close(sockfd);      //não deixa receber mais ligações
    
    for(int i = 0; i<MAX_CLIENTS && workers[i]!=0; i++) {       //espera que threads acabem os trabalhos dos clientes
        join=pthread_join(workers[i], NULL);
        if(join){
            perror("Can't join thread");
        }
    }

    TIMER_READ(stopTime);
    print_tecnicofs_tree(outputFp, fs);
    fprintf(outputFp, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, stopTime));
    fflush(outputFp);
    fclose(outputFp);
    free_tecnicofs(fs);

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

    exit(EXIT_SUCCESS); //nunca usado
}
