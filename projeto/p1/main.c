#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
tecnicofs* fs;
FILE *inputfile, *outputfile;
pthread_mutex_t lock1;


char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

/*Mostra formato esperado de input*/
static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile numthreads\n", appName);
    exit(EXIT_FAILURE);
}

/*Recebe os argumentos do programa*/
static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
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
}

/*se ainda ha' espaço no vetor de comandos, adiciona mais um comando*/
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

    /*percorre as linhas do ficheiro e manda incerir comandos no vetor*/
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

void applyCommands(){    //devolve o tempo de execucao
    while(numberCommands > 0){  //percorre os comandos no vetor
        pthread_mutex_lock(&lock1);
        const char* command = removeCommand();

        if (command == NULL){
            continue;
        }
    
        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                pthread_mutex_unlock(&lock1);
                create(fs, name, iNumber);
                break;
            case 'l':
                pthread_mutex_unlock(&lock1);
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                pthread_mutex_unlock(&lock1);
                delete(fs, name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

//argc = numero de argumentos; argv = lista de argumentos
int main(int argc, char* argv[]) {
    float tempoExec, clock0, clock1;
    pthread_t *tid=malloc(sizeof(pthread_t)*numberThreads);;

    //recebe input
    parseArgs(argc, argv);

    //cria novo fileSystem
    fs = new_tecnicofs();
    //processa o input
    processInput();

    //pthread_mutex_lock(&lock1);
    //pthread_mutex_unlock(&lock1);

    pthread_mutex_init(&lock1, NULL);
    clock0=clock();
    for(int i=0; i<numberThreads;i++){
        pthread_create(&tid[i],0,applyCommands,NULL);
    }

    //espera q acabem as threads todas
    for(int i = 0; i<numberThreads; i++){
        pthread_join(tid[i], NULL);
    }
    clock1=clock();
    free(tid);
    tempoExec = (clock1-clock0)/CLOCKS_PER_SEC;





    //exporta a arvore e tempo de execução para um ficheiro    
    print_tecnicofs_tree(outputfile, fs, tempoExec);

    //fecha o output
    fflush(outputfile);
    fclose(outputfile);

    //liberta a memoria do fileSystem
    free_tecnicofs(fs);
    
    exit(EXIT_SUCCESS);
}
