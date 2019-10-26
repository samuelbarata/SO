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

char inputCommands[VECTOR_SIZE][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;
int tailQueue = 0;

pthread_mutex_t commandsLock;
sem_t semVetor;

tecnicofs** fs;

static void displayUsage (const char* appName){
    printf("Usage: %s input_filepath output_filepath numthreads[>=1] numbuckets[>=1]\n", appName);
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

}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        sem_wait(&semVetor);

        mutex_lock(&commandsLock);
        strcpy(inputCommands[(tailQueue++)%VECTOR_SIZE], data);
        numberCommands++;
        mutex_unlock(&commandsLock);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[(headQueue++)%VECTOR_SIZE];
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

void* applyCommands(void* stop){
    while(!(*(int*)stop) || numberCommands>0){
        char token;
        char name[MAX_INPUT_SIZE], newName[MAX_INPUT_SIZE];
        int iNumber;
        
        mutex_lock(&commandsLock);
        const char* command = removeCommand();
        sem_post(&semVetor);

        if (command == NULL){
            mutex_unlock(&commandsLock);
            continue;
        }
        
        sscanf(command, "%c %s %s", &token, name, newName);

        switch (token) {
            case 'r':
                delete(fs, name);
                strcpy(name, newName);
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
            default: { /* error */
                mutex_unlock(&commandsLock);
                fprintf(stderr, "Error: commands to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

void runThreads(FILE* timeFp){
    TIMER_T startTime, stopTime;
    sem_init(&semVetor, 0, VECTOR_SIZE);
    int err, *stop=malloc(sizeof(int)), join;
    *stop=0;
    pthread_t* workers = (pthread_t*) malloc((numberThreads+1) * sizeof(pthread_t));


    TIMER_READ(startTime);

    for(int i = 0; i < numberThreads+1; i++){
        if(!i)  //processInput
            err = pthread_create(&workers[i], NULL, processInput, NULL);
        else    //applyCommands
            err = pthread_create(&workers[i], NULL, applyCommands, (void*)stop);
        
        if (err != 0){
            perror("Can't create thread");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < numberThreads+1; i++) {
        join=pthread_join(workers[i], NULL);
        if(!i)  //processInput
            *stop=1;    //applyCommands pode parar
        if(join){
            perror("Can't join thread");
        }
    }

    TIMER_READ(stopTime);
    fprintf(timeFp, "TecnicoFS completed in %.4f seconds.\n", TIMER_DIFF_SECONDS(startTime, stopTime));
    free(stop);
    free(workers);
    sem_destroy(&semVetor);
}


int main(int argc, char* argv[]) {
    parseArgs(argc, argv);
    mutex_init(&commandsLock);

    FILE * outputFp = openOutputFile();
    fs = new_tecnicofs();
    runThreads(stdout);     //numberThreads Threads

    print_tecnicofs_tree(outputFp, fs);
    fflush(outputFp);
    fclose(outputFp);

    mutex_destroy(&commandsLock);
    free_tecnicofs(fs);
    exit(EXIT_SUCCESS);
}
