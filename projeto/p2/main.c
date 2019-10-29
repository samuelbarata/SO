#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "lib/timer.h"
#include "lib/bst.h"
#include "globals.h"
#include "fs.h"
#include "sync.h"

char* global_inputFile = NULL;
char* global_outputFile = NULL;
int numberThreads = 0;
int numberBuckets = 0;
int stop = 0;

char inputCommands[ARRAY_SIZE][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;
int tailQueue = 0;

pthread_mutex_t commandsLock;
sem_t canProduce, canRemove;

tecnicofs** fs;

static void displayUsage (const char* appName){
    fprintf(stderr, "Usage: %s input_filepath output_filepath numthreads[>=1] numbuckets[>=1]\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }

    global_inputFile = argv[1];
    global_outputFile = argv[2];
    numberThreads = atoi(argv[3]);
    numberBuckets = atoi(argv[4]);
    if (numberThreads<=0 || numberBuckets<=0) {
        fprintf(stderr, "Invalid number of threads\n");
        displayUsage(argv[0]);
    }
    #ifndef SYNC
        numberThreads=1;
    #endif
}

int insertCommand(char* data) {
    se_wait(&canProduce);
    strcpy(inputCommands[(tailQueue++)%ARRAY_SIZE], data);
    numberCommands++;
    se_post(&canRemove);
    return 1;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[(headQueue++)%ARRAY_SIZE];
    }
    return NULL;
}

void errorParse(int lineNumber){
    fprintf(stderr, "Error: line %d invalid\n", lineNumber);
    exit(EXIT_FAILURE);
}

void *processInput(){
    FILE* inputFile;
    inputFile = fopen(global_inputFile, "r");
    if(!inputFile){
        fprintf(stderr, "Error: Could not read %s\n", global_inputFile);
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
            case 'c':   //create
            case 'l':   //lookup
            case 'd':   //delete
                if(numTokens != 2)
                    errorParse(lineNumber);
                if(insertCommand(line))
                    break;
                return NULL;
            case 'r':   //rename
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
    fclose(inputFile);
    insertCommand("q");
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
        char name[MAX_INPUT_SIZE], newName[MAX_INPUT_SIZE];
        int iNumber, newInumber;
        
        se_wait(&canRemove);
        mutex_lock(&commandsLock);
        const char* command = removeCommand();
        
        if(stop){  //nao ha mais comandos   
            mutex_unlock(&commandsLock);
            se_post(&canRemove);
            pthread_exit(NULL);
        }
        else if (command == NULL){
            mutex_unlock(&commandsLock);
            se_post(&canRemove);
            continue;
        }
        
        sscanf(command, "%c %s %s", &token, name, newName);
        se_post(&canProduce);

        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                mutex_unlock(&commandsLock);
                create(fs, name, iNumber);
                break;
            case 'l':
                mutex_unlock(&commandsLock);
                int searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                mutex_unlock(&commandsLock);
                delete(fs, name);
                break;
            case 'r':
                iNumber = lookup(fs, name);         //inumber do ficheiro atual
                newInumber = lookup(fs, newName);   //ficheiro novo existe?
                mutex_unlock(&commandsLock);
                if(iNumber && !newInumber){ //se rename se inumber != 0 e inumber novo == 0
                    delete(fs, name);
                    create(fs, newName, iNumber);
                }
                break;
            case 'q':
                stop=1;
                se_post(&canRemove);
                mutex_unlock(&commandsLock);
                pthread_exit(NULL);
                
            default: { /* error */
                mutex_unlock(&commandsLock);
                fprintf(stderr, "Error: could not apply command %c\n", token);
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

void initSemaforos(){
    se_init(&canProduce, ARRAY_SIZE);   //inicialmente ARRAY_SIZE vagas no array
    se_init(&canRemove, 0);             //Array inicialmente vazio
}

void runThreads(FILE* timeFp){
    TIMER_T startTime, stopTime;
    
    int err, join;
    pthread_t* workers = (pthread_t*) malloc((numberThreads+1) * sizeof(pthread_t));
    
    initSemaforos();

    TIMER_READ(startTime);
    for(int i = 0; i < numberThreads+1; i++){
        if(!i)  //processInput
            err = pthread_create(&workers[i], NULL, processInput, NULL);
        else    //applyCommands
            err = pthread_create(&workers[i], NULL, applyCommands, NULL);
        
        if (err){
            perror("Can't create thread");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < numberThreads+1; i++) {
        join=pthread_join(workers[i], NULL);
        if(join){
            perror("Can't join thread");
        }
    }

    TIMER_READ(stopTime);
    fprintf(timeFp, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, stopTime));
    free(workers);
    se_destroy(&canProduce);
    se_destroy(&canRemove);
}


int main(int argc, char* argv[]) {
    parseArgs(argc, argv);
    mutex_init(&commandsLock);

    FILE * outputFp = openOutputFile();
    fs = new_tecnicofs();
    runThreads(stdout);

    print_tecnicofs_tree(outputFp, fs);
    fflush(outputFp);
    fclose(outputFp);

    mutex_destroy(&commandsLock);
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}
