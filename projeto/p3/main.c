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

int numberCommands = 0; //numero de comandos a processar

pthread_mutex_t commandsLock;   //lock do vetor de comandos
sem_t canProduce, canRemove;

FILE* outputFp=NULL;

tecnicofs* fs;

static void displayUsage (const char* appName){
    fprintf(stderr, "Usage: %s nomesocket output_filepath numbuckets[>=1]\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }

    global_SocketName = argv[1];
    global_outputFile = argv[2];
    numberBuckets = atoi(argv[3]);
    if (numberBuckets<=0) {
        fprintf(stderr, "Invalid number of buckets\n");
        displayUsage(argv[0]);
    }
}

void errorParse(int lineNumber){
    fprintf(stderr, "Error: line %d invalid\n", lineNumber);
    exit(EXIT_FAILURE);
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

int applyCommands(char* command, uid_t user){
        char token;
        char arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
        int iNumber, newiNumber;

        if (command == NULL)
            return 0;
        
        sscanf(command, "%c %s %s", &token, arg1, arg2);

        switch (token) {
            case 'c':
                return create(fs, arg1, user, permConv(arg2)); //FIXME: 0: user
				break;
            case 'd':
                return delete(fs, arg1, user);
                break;
            case 'r':
                return reName(fs, arg1, arg2, iNumber);
                break;
            case 'l':
				return readFromFile(fs, arg1, arg2);
				break;
            case 'o':
                return openFile(fs, arg1, arg2);
                break;
            case 'x':
                return closeFile(fs, arg1);
                break;
            case 'w':
                return writeToFile(fs, arg1, arg2);
                break;
                
            default: { /* error */
                fprintf(stderr, "Error: could not apply command %c\n", token);  //TODO: devolver erro comando not valid em vez de crashar server
                exit(EXIT_FAILURE);
            }
        }
    return 0;
}

void inits(){ 
    struct sockaddr_un serv_addr;
    int servlen;

    srand(time(NULL));

    /* Cria socket stream */
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		perror("server: can't open stream socket");

    /*Elimina o nome, para o caso de já existir.*/
	unlink(global_SocketName);

    bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, global_SocketName);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	if (bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
		perror("server: can't bind local address");
	
	listen(sockfd, 5);

}


void *newClient(void* cli){
	sigset_t set;   //exemplo manual linux
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	if(pthread_sigmask(SIG_SETMASK, &set, NULL)){
		perror("sig_mask: Failure");
		pthread_exit(NULL);
	}
	client *cliente = (client*)cli;
	printf("socket: %02d\n",cliente->socket);
    
	int n, res;
	char line[MAX_INPUT_SIZE];
	int error_code=0;
	unsigned int error_code_size = sizeof(error_code);
	while(TRUE) {   //FIXME: check if socket still connected
		n = recv(cliente->socket, line, MAX_INPUT_SIZE, 0);
		if (n == 0)
			break;
		else if (n < 0){
			perror("read from socket");
			free(cliente);
			pthread_exit(NULL);
		}
		printf("%s\n",line);
		res = applyCommands(line, cliente->uid);
		n = dprintf(cliente->socket, "%d", res);
		if(n < 0){
			perror("dprintf");
		}
	}
	printf("exit: %02d\n",cliente->socket);
	free(cliente);
	return NULL;
}

void connections(){
    int clients=0, err;
    unsigned int ucred_len;

    struct ucred ucreds;    

    int newsockfd, clilen;
	struct sockaddr_un cli_addr;
    client *cliente;

    for(;;){
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd,(struct sockaddr*) &cli_addr,(socklen_t*) &clilen);
		if(newsockfd < 0)
			perror("server: accept error");

        cliente = malloc(sizeof(client));

		ucred_len = sizeof(struct ucred);
        if(getsockopt(newsockfd, SOL_SOCKET, SO_PEERCRED, &ucreds, &ucred_len) == -1){
			perror("cant get client uid");
			free(cliente);
			continue;
		}
        cliente->socket=newsockfd;
        cliente->pid=ucreds.pid;
        cliente->uid=ucreds.uid;

        for(int i = 0; i < USER_ABERTOS; i++){
            client -> abertos[i] = -1;
        }

        clients++;
        workers = realloc(workers,sizeof(pthread_t*)*clients+1);
        workers[clients]='\0';  //marca o fim do array

        err = pthread_create(&workers[clients-1], NULL, newClient, (void*)cliente);
        if (err){
            perror("Can't create thread");
            exit(EXIT_FAILURE);
        }
	}
}

void exitServer(){
    int join;
    pthread_t *pointer; //percorre os workers
    close(sockfd);      //não deixa receber mais ligações
    for(pointer=workers; pointer!='\0'; pointer++) {       //espera que threads acabem os trabalhos dos clientes
        join=pthread_join(*pointer, NULL);
        if(join){
            perror("Can't join thread");
        }
    }
    print_tecnicofs_tree(outputFp, fs);
    fflush(outputFp);
    fclose(outputFp);

    free_tecnicofs(fs);

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
