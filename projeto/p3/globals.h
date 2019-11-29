/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define USER_ABERTOS		5
#define FILE_CLOSED 		-1
#define MAX_CLIENTS			100
#define MAX_INPUT_SIZE		1024
#define DELAY				5000
#define FALSE				0
#define TRUE				1
#define MINGUA_CONSTANT 	0.0001
#define CODE_SIZE			4			//maior codigo == "-11\0"

//#define USER_CAN_READ		0b00000001
//#define USER_CAN_WRITE		0b00000010
#define OPEN_USER_READ		0b00000100
#define OPEN_USER_WRITE		0b00001000
#define	OPEN_OTHER_READ		0b00010000
#define OPEN_OTHER_WRITE	0b00100000
#define ESPACO_AVAILABLE	0b01000000

#include <sys/types.h>
#include <stdlib.h>
#include "tecnicofs-api-constants.h"
#include "sync.h"

typedef struct ficheiro{
	int fd;
	permission mode;
} ficheiro;

typedef struct client{
	int socket;
	uid_t uid;
	ficheiro ficheiros[USER_ABERTOS];
	syncMech lock;
} client;

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifdef DEBUG
	#define DEBUG_TEST 1
#else
	#define DEBUG_TEST 0
#endif

//debug print
#define debug_print(fmt, ...) do { if (DEBUG_TEST) fprintf(stdout, fmt, __VA_ARGS__);fflush(stdout);} while (0)

#endif /* CONSTANTS_H */
