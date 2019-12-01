#ifndef SAFE_H
#define SAFE_H

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef enum origin {IGNORE, MAIN, THREAD} origin;

void terminate(origin _origin);
void *safe_malloc(size_t __size, origin _origin);
void *safe_strdup(const char *__s1, origin _origin);
int safe_socket(int domain, int type, int protocol);
void safe_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
void safe_sigmask(int how, const sigset_t *set, sigset_t *oldset);
int safe_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
void safe_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
void safe_pthread_join(pthread_t, void *_Nullable, origin _origin);

#endif