#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int numberCommands = 0; //numero de comandos a processar

pthread_mutex_t commandsLock;   //lock do vetor de comandos
sem_t canProduce, canRemove;

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

char* removeCommand() {
    mutex_lock(&commandsLock);
    if(numberCommands > 0){
        numberCommands--;
        return NULL;
    }
    return NULL;
}

void errorParse(int lineNumber){
    fprintf(stderr, "Error: line %d invalid\n", lineNumber);
    exit(EXIT_FAILURE);
}

void *processInput(){
    FILE* inputFile;
    inputFile = fopen(global_SocketName, "r");
    if(!inputFile){
        fprintf(stderr, "Error: Could not read %s\n", global_SocketName);
        exit(EXIT_FAILURE);
    }
    char line[MAX_INPUT_SIZE];
    int lineNumber = 0;

    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token;
        char name[MAX_INPUT_SIZE], newName[MAX_INPUT_SIZE];
        lineNumber++;

        int numTokens = sscanf(line, "%c %s %s", &token, name, newName);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'd':   //delete
            case 'x':   //close
                if(numTokens != 2)
                    errorParse(lineNumber);
                if(insertCommand(line))
                    break;
                return NULL;
            case 'c':   //create
            case 'r':   //rename
            case 'o':   //open
            case 'l':   //read
            case 'w':   //write
                if(numTokens != 3)
                    errorParse(lineNumber);
                if(insertCommand(line))
                    break;
                return NULL;
            case '#':   //comment
                break;
            default: { /* error */
                errorParse(lineNumber);
            }
        }
    }
    insertCommand("q exit exit\n");     //comando a ser inserido em ultimo lugar
    fclose(inputFile);
    return NULL;
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

void* applyCommands(){
    while(1){
        char token;
        char arg1[MAX_INPUT_SIZE], arg2[MAX_INPUT_SIZE];
        int iNumber, newiNumber;
        
        const char* command = removeCommand();
        
        if(stop){  //nao ha mais comandos   
            mutex_unlock(&commandsLock);
            se_post(&canRemove);
            return NULL;
        }
        else if (command == NULL){
            mutex_unlock(&commandsLock);
            se_post(&canRemove);
            continue;
        }
        
        sscanf(command, "%c %s %s", &token, arg1, arg2);
        se_post(&canProduce);

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
            
            case 'q':       //nao ha mais comandos a ser processados
                stop=1;     //as threads seguintes podem parar
                se_post(&canRemove);
                mutex_unlock(&commandsLock);
                return NULL;
                break;
                
            default: { /* error */
                mutex_unlock(&commandsLock);
                fprintf(stderr, "Error: could not apply command %c\n", token);
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

void inits(){
    mutex_init(&commandsLock);
    srand(time(NULL));
}

void destroys(){
    mutex_destroy(&commandsLock);
}

void *newClient(){


    return NULL;
}

void initSocket(){
    pthread_t* workers=NULL;
    int clients=0, err;

    int sockfd, newsockfd, clilen, childpid, servlen;
	struct sockaddr_un cli_addr, serv_addr;
    /* Cria socket stream */
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		err_dump("server: can't open stream socket");

    /*Elimina o nome, para o caso de já existir.*/
	unlink(global_SocketName); 
    
    bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, global_SocketName);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
	if (bind(sockfd, (struct sockaddr*) &serv_addr, servlen) < 0)
		err_dump("server, can't bind local address");
	
	listen(sockfd, 5);

    for(;;){
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
		if(newsockfd < 0)
			err_dump("server:accepterror");
		
        clients++;
        workers = realloc(workers,sizeof(pthread_t*)*clients+1);
        workers[clients]=NULL;  //marca o fim do array

        err = pthread_create(&workers[clients-1], NULL, newClient, NULL);
        if (err){
            perror("Can't create thread");
            exit(EXIT_FAILURE);
        }
	}
}

void exitServer(pthread_t* workers, int newsockfd){
    int join;
    pthread_t *pointer; //percorre os workers
    for(pointer=workers; *pointer!=NULL; pointer++) {
        join=pthread_join(*pointer, NULL);
        if(join){
            perror("Can't join thread");
        }
    }
    close(newsockfd);
    exit(EXIT_SUCCESS);
}






int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

    FILE * outputFp = openOutputFile();
    fs = new_tecnicofs();
    runThreads(stdout);

    print_tecnicofs_tree(outputFp, fs);
    fflush(outputFp);
    fclose(outputFp);

    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}
