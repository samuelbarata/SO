#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include "definer.h"
#include "fs.h"
#include "lib/hash.h"



tecnicofs** fs;

FILE *inputfile, *outputfile;

pthread_mutex_t mutexVectorLock;
pthread_rwlock_t rwVectorLock;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

int sleeptime;

/*Mostra formato esperado de input*/
static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numthreads numbuckets\n", appName);
    exit(EXIT_FAILURE);
}

/*Recebe os argumentos do programa*/
static void parseArgs (long argc, char* const argv[]){
    if (argc > 6) { //mudei de 5 para 6
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
    inputfile = fopen(argv[1], "r");
    if(!inputfile){
        fprintf(stderr, "Input file not found:\n");
        displayUsage(argv[0]);
    }
    outputfile = fopen(argv[2], "w");
    numberThreads = atoi(argv[3]);
    numberBuckets = atoi(argv[4]);
    if (argc == 5){
        sleeptime = 0;
    }
    else{
        sleeptime = atoi(argv[5]);
    }
}

/*se ainda ha espaço no vetor de comandos, adiciona mais um comando*/
int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

/*remove e devolve o primeiro comando da lista*/
char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

/*quando o comando introduzido n e' reconhecido*/
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    //exit(EXIT_FAILURE);
}

/*le e adiciona comandos ao vetor*/
void processInput(){
    char line[MAX_INPUT_SIZE];

    /*percorre as linhas do ficheiro e manda inserir comandos no vetor*/
    while (fgets(line, sizeof(line)/sizeof(char), inputfile)) {
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(inputfile);
}

void *applyCommands(){    //devolve o tempo de execucao 
    while(numberCommands > 0){  //percorre os comandos no vetor
        char token;
        char name[MAX_INPUT_SIZE];
        int searchResult;
        int iNumber;
        lock_mutex(&mutexVectorLock);
        lock_rw(&rwVectorLock);
        const char* command = removeCommand();
        int hhash;

        if (command == NULL){
            unlock_rw(&rwVectorLock);
            unlock_mutex(&mutexVectorLock);
            continue;
        }

        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        hhash = hash(name, numberBuckets);
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs[hhash]);
                unlock_rw(&rwVectorLock);
                unlock_mutex(&mutexVectorLock);
                lock_mutex(&fs[hhash]->treeMutexLock);
                lock_rw(&fs[hhash]->treeRwLock);

                create(fs, name, iNumber);
                unlock_rw(&fs[hhash]->treeRwLock);
                unlock_mutex(&fs[hhash]->treeMutexLock);

                break;

            case 'l':
                unlock_rw(&rwVectorLock);
                unlock_mutex(&mutexVectorLock);


                lock_mutex(&fs[hhash]->treeMutexLock);
                lock_r(&fs[hhash]->treeRwLock);

                searchResult = lookup(fs, name);
                unlock_rw(&fs[hhash]->treeRwLock);
                unlock_mutex(&fs[hhash]->treeMutexLock);

                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;

            case 'd':
                unlock_rw(&rwVectorLock);
                unlock_mutex(&mutexVectorLock);

                lock_mutex(&fs[hhash]->treeMutexLock);
                lock_rw(&fs[hhash]->treeRwLock);


                delete(fs, name);
                unlock_rw(&fs[hhash]->treeRwLock);
                unlock_mutex(&fs[hhash]->treeMutexLock);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    struct timeval clock0, clock1;
    //recebe input
    parseArgs(argc, argv);

    //fazer o sleep
    sleep(sleeptime);




    //cria novo fileSystem
    fs = new_tecnicofs();
    //processa o input
    processInput();
    
    #ifdef DMUTEX
        pthread_mutex_init(&mutexVectorLock, NULL);
        pthread_mutex_init(&treeMutexLock, NULL);
    #elif DRWLOCK
        pthread_rwlock_init(&rwVectorLock, NULL);
        pthread_rwlock_init(&treeRwLock, NULL);
    #endif

    //se forem 0 threads passa para um
    if(numberThreads < 0)
        numberThreads=1;
    
    pthread_t tid[numberThreads];

    gettimeofday(&clock0, NULL);
    //inicia $numberThreads Threads
    for(int i=0; i<numberThreads; i++){
        if(pthread_create(&tid[i], 0, applyCommands, NULL) != 0)
            fprintf(stderr, "Error creating thread\n");
    }

    //espera que acabem todas as threads
    for(int i = 0; i<numberThreads; i++){
        pthread_join(tid[i], NULL);
    }

    gettimeofday(&clock1, NULL);
 
    //exporta a arvore para um ficheiro
    print_tecnicofs_tree(outputfile, fs);
    
    double elapsed = (clock1.tv_sec - clock0.tv_sec) + 
              ((clock1.tv_usec - clock0.tv_usec)/1000000.0);

    fprintf(stdout, "TecnicoFS completed in %.04f seconds.\n",
    elapsed);

    //fecha o output
    fflush(outputfile);
    fclose(outputfile);

    //liberta a memoria do fileSystem
    free_tecnicofs(fs);
    
    exit(EXIT_SUCCESS);
}
