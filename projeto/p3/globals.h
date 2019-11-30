/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define FALSE				0
#define TRUE				1

#define MAX_OPEN_FILES		5
#define MAX_CLIENTS			100
#define MAX_INPUT_SIZE		1024

#define FILE_CLOSED 		-1
#define DELAY				5000
#define MINGUA_CONSTANT 	0.0001
#define CODE_SIZE			4			//maior codigo == "-11\0"

//#define USER_CAN_READ		0b000000001
//#define USER_CAN_WRITE	0b000000010

#define OPEN_USER_READ		0b000000100
#define OPEN_USER_WRITE		0b000001000
#define	OPEN_OTHER_READ		0b000010000
#define OPEN_OTHER_WRITE	0b000100000
#define ESPACO_AVAILABLE	0b001000000
#define USER_IS_OWNER		0b010000000
#define OPEN_USER			(OPEN_USER_READ | OPEN_USER_WRITE)
#define OPEN_OTHER			(OPEN_OTHER_READ | OPEN_OTHER_WRITE)
#define OPEN_ANY			(OPEN_USER | OPEN_OTHER)

#include <sys/types.h>
#include <stdlib.h>
#include "tecnicofs-api-constants.h"
#include "sync.h"

typedef struct ficheiro{
	char* key;
	int fd;
	permission mode;
} ficheiro;

typedef struct client{
	int socket;
	uid_t uid;
	ficheiro ficheiros[MAX_OPEN_FILES];
	syncMech lock;
} client;

#ifdef DEBUG
	#define DEBUG_TEST 1
#else
	#define DEBUG_TEST 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
/*Prints debug information to stdout
Uses similar syntax to fprintf*/
#define debug_print(...) do { if (DEBUG_TEST) fprintf(stdout, __VA_ARGS__);fflush(stdout);} while (0)

#endif /* CONSTANTS_H */
