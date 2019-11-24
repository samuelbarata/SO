/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define ARRAY_SIZE 10
#define MAX_INPUT_SIZE 100
#define DELAY 5000
#define FALSE 0
#define TRUE 1
#define MINGUA_CONSTANT 0.0001

#include <stdlib.h>
#include <stdio.h>
void errorLog(char* A){fprintf(stderr,"%s", A);exit(EXIT_FAILURE);}

#endif /* CONSTANTS_H */
