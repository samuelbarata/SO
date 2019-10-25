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
#include "lib/timer.h"

tecnicofs** fs;

FILE *inputfile, *outputfile;

syncMech vectorLock;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

/*Mostra formato esperado de input*/
static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numthreads numbuckets\n", appName);
    exit(EXIT_FAILURE);
}

/*Recebe os argumentos do programa*/
static void parseArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
    inputfile = fopen(argv[1], "r");
    if(!inputfile){
        fprintf(stderr, "Input file not found:\n");
        displayUsage(argv[0]);
    }
    outputfile = fopen(argv[2], "w");
    if (!outputfile) {
        fprintf(stderr, "Output file could not be created\n");
        exit(EXIT_FAILURE);
    }
    numberThreads = atoi(argv[3]);
    if(numberThreads<=0){
        fprintf(stderr, "Invalid number of threads\n");
        displayUsage(argv[0]);
    }
    numberBuckets = atoi(argv[4]);
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
void errorParse(int lineNumber){
    fprintf(stderr, "Error: line %d invalid\n", lineNumber);
    exit(EXIT_FAILURE);
}

/*le e adiciona comandos ao vetor*/
void processInput(){
    char line[MAX_INPUT_SIZE];
    int lineNumber = 0;

    /*percorre as linhas do ficheiro e manda inserir comandos no vetor*/
    while (fgets(line, sizeof(line)/sizeof(char), inputfile)) {
        char token;
        char name[MAX_INPUT_SIZE];
        lineNumber++;

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
                    errorParse(lineNumber);
                if(insertCommand(line))
                    break;
                return;
            case '#':
                break;
            default: { /* error */
                errorParse(lineNumber);
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
        sync_rw_lock(&vectorLock);
        const char* command = removeCommand();
        int hhash;

        if (command == NULL){
            sync_unlock(&vectorLock);
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
                sync_unlock(&vectorLock);
                sync_rw_lock(&fs[hhash]->treeLock);

                create(fs, name, iNumber);
                sync_unlock(&fs[hhash]->treeLock);

                break;

            case 'l':
                sync_unlock(&vectorLock);
                sync_r_lock(&fs[hhash]->treeLock);
                searchResult = lookup(fs, name);
                sync_unlock(&fs[hhash]->treeLock);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;

            case 'd':
                sync_unlock(&vectorLock);

                sync_rw_lock(&fs[hhash]->treeLock);

                delete(fs, name);
                sync_unlock(&fs[hhash]->treeLock);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                sync_unlock(&fs[hhash]->treeLock);
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

void runThreads(FILE* timeFp){
    TIMER_T startTime, stopTime;
    #if defined (RWLOCK) || defined (MUTEX)
        pthread_t* workers = (pthread_t*) malloc(numberThreads * sizeof(pthread_t));
    #endif

    TIMER_READ(startTime);
    #if defined (RWLOCK) || defined (MUTEX)
        for(int i = 0; i < numberThreads; i++){
            int err = pthread_create(&workers[i], NULL, applyCommands, NULL);
            if (err != 0){
                perror("Can't create thread");
                exit(EXIT_FAILURE);
            }
        }
        for(int i = 0; i < numberThreads; i++) {
            if(pthread_join(workers[i], NULL)) {
                perror("Can't join thread");
            }
        }
    #else
        applyCommands();
    #endif
    TIMER_READ(stopTime);
    fprintf(timeFp, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, stopTime));
    #if defined (RWLOCK) || defined (MUTEX)
        free(workers);
    #endif
}

int main(int argc, char* argv[]) {
    //recebe input
    parseArgs(argc, argv);
    //cria novo fileSystem
    fs = new_tecnicofs();
    //processa o input
    processInput();
    
    sync_init(&vectorLock, NULL);
    
    runThreads(stdout);
 

    //fecha o output
    fflush(outputfile);
    fclose(outputfile);

    //liberta a memoria do fileSystem
    free_tecnicofs(fs);
    
    exit(EXIT_SUCCESS);
}
