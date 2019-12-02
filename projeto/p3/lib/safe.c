#include "safe.h"

void terminate(origin _origin){
	switch (_origin){
	case IGNORE:
		break;
	case MAIN:
		raise(SIGINT);
		break;
	case THREAD:
		pthread_exit(NULL);
		break;
	default:
		exit(EXIT_FAILURE);
		break;
	}
}

void* safe_malloc(size_t __size,origin _origin){
	void* p;
	p = malloc(__size);
	if(!p){
		perror("malloc failed");
		terminate(_origin);
	}
	return p;
}

void *safe_strdup(const char *__s1, origin _origin){
	char* p = strdup(__s1);
	if(!p){
		perror("strdup failed");
		terminate(_origin);
	}
	return p;
}

int safe_socket(int domain, int type, int protocol){
	int sockfd = socket(domain, type, protocol);
	if(sockfd<0){
		perror("socket: can't open socket");
		terminate(MAIN);
	}
	return sockfd;
}

void safe_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
	int aux = bind(sockfd, addr, addrlen);
	if(aux<0){
		perror("bind: can't bind local address");
		terminate(MAIN);
	}
}

void safe_sigmask(int how, const sigset_t *set, sigset_t *oldset){
	int aux = pthread_sigmask(how, set, oldset);
	if(aux<0){
		perror("sig_mask: Failure");
		terminate(THREAD);
	}
}

int safe_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
	int aux = accept(sockfd, addr, addrlen);
	if(aux<0){
		perror("accept");
		terminate(MAIN);
	}
	return aux;
}

void safe_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg){
	int aux = pthread_create(thread, attr, start_routine, arg);
	if (aux){
		perror("Can't create thread");
		terminate(MAIN);
	}
}

void safe_pthread_join(pthread_t thread, void *p, origin _origin){
	int aux = pthread_join(thread, p);
	if(aux){
		perror("Can't join thread");
		terminate(_origin);
	}
}

