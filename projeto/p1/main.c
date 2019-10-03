#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "fs.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

FILE *inputfile, *outputfile;

/*Mostra formato esperado de input*/
static void displayUsage (const char* appName){
    printf("Usage: %s inputfile outputfile\n", appName);
    exit(EXIT_FAILURE);
}

/*Recebe os argumentos do programa*/
static void parseArgs (long argc, char* const argv[]){
    if (argc != 3) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
    inputfile = fopen(argv[1], "r");
    if(!inputfile){
        fprintf(stderr, "Input file not found:\n");
        displayUsage(argv[0]);
    }
    outputfile = fopen(argv[2], "a");
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

float applyCommands(){    //devolve o tempo de execucao
    float clock0 = clock(), clock1;
    while(numberCommands > 0){  //percorre os comandos no vetor
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
                create(fs, name, iNumber);
                break;
            case 'l':
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                delete(fs, name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    clock1 = clock();
    return (clock1-clock0)/CLOCKS_PER_SEC;
}

//argc = numero de argumentos; argv = lista de argumentos
int main(int argc, char* argv[]) {
    float tempoExec;
    //recebe input
    parseArgs(argc, argv);

    //cria novo fileSystem
    fs = new_tecnicofs();
    //processa o input
    processInput();

    //aplica os comandos e devolve o tempo de execucao
    tempoExec = applyCommands();
    //exporta a arvore e tempo de execução para um ficheiro    
    print_tecnicofs_tree(outputfile, fs, tempoExec);

    //fecha o output
    fflush(outputfile);
    fclose(outputfile);

    //liberta a memoria do fileSystem
    free_tecnicofs(fs);
    
    exit(EXIT_SUCCESS);
}
