/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define ARRAY_SIZE 10
#define MAX_INPUT_SIZE 100
#define DELAY 5000
#define FALSE 0
#define TRUE 1
#define MINGUA_CONSTANT 0.0001

#include <sys/types.h>
typedef struct client{
	int socket;
	pid_t pid;
	uid_t uid;
} client;

#endif /* CONSTANTS_H */
