#ifndef SAFE_H
#define SAFE_H

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>

typedef enum origin {MAIN, THREAD} origin;

void terminate(origin _origin);
void *safe_malloc(size_t __size, origin _origin);
void *safe_strdup(const char *__s1, origin _origin);

#endif