#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "lib/timer.h"
#include "lib/inodes.h"
#include "globals.h"
#include "fs.h"
#include "sync.h"
#include "unix.h"

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

void* applyCommands(char* command){
        char token;
        char arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
        int iNumber, newiNumber;
        
        //const char* command = removeCommand();

        if (command == NULL){
            mutex_unlock(&commandsLock);
            return NULL;
        }
        
        sscanf(command, "%c %s %s", &token, arg1, arg2);

        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                mutex_unlock(&commandsLock);
                create(fs, arg1, iNumber, 0, permConv(arg2)); //FIXME: 0: owner
                break;
            case 'l':
                mutex_unlock(&commandsLock);
                int searchResult = lookup(fs, arg1);
                if(!searchResult)
                    printf("%s not found\n", arg1);
                else
                    printf("%s found with inumber %d\n", arg1, searchResult);
                break;
            case 'd':
                mutex_unlock(&commandsLock);
                delete(fs, arg1);
                break;
            case 'r':
            	iNumber = lookup(fs, arg1);         //inumber do ficheiro atual
                newiNumber = lookup(fs, arg2);      //ficheiro novo existe?
                mutex_unlock(&commandsLock);
                if(iNumber && !newiNumber)
                    reName(fs, arg1, arg2, iNumber);
                break;
                
            default: { /* error */
                mutex_unlock(&commandsLock);
                fprintf(stderr, "Error: could not apply command %c\n", token);  //TODO: devolver erro comando not valid em vez de crashar server
                exit(EXIT_FAILURE);
            }
        }
    return NULL;
}

void inits(){ 
    struct sockaddr_un serv_addr;
    int servlen;

    mutex_init(&commandsLock);
    srand(time(NULL));

    /* Cria socket stream */
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		errorLog("server: can't open stream socket");

    /*Elimina o nome, para o caso de já existir.*/
	unlink(global_SocketName);

    bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, global_SocketName);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	if (bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
		errorLog("server: can't bind local address");
	
	listen(sockfd, 5);

}


void *newClient(void* socket){
    int socketfd = (int)socket;
    printf("socket: %d\n",socketfd);

    return NULL;
}

void connections(){
    int clients=0, err;

    int newsockfd, clilen, childpid;
	struct sockaddr_un cli_addr;

    for(;;){
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
		if(newsockfd < 0)
			errorLog("server: accept error");
		
        clients++;
        workers = realloc(workers,sizeof(pthread_t*)*clients+1);
        workers[clients]=NULL;  //marca o fim do array

        err = pthread_create(&workers[clients-1], NULL, newClient, (void*)newsockfd);
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
    for(pointer=workers; *pointer!=NULL; pointer++) {       //espera que threads acabem os trabalhos dos clientes
        join=pthread_join(*pointer, NULL);
        if(join){
            perror("Can't join thread");
        }
    }
    mutex_destroy(&commandsLock);
    
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
