#ifndef SAFE_H
#define SAFE_H

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>

typedef enum origin { MAIN, THREAD} origin;

void *safe_malloc(size_t __size, origin _origin);

#endif