/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define DELAY 5000

char* global_inputFile = NULL;
char* global_outputFile = NULL;
int numberThreads = 0;
pthread_mutex_t commandsLock;
tecnicofs* fs;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;
int numberBuckets = 0;






#endif /* CONSTANTS_H */
