#include "safe.h"

void terminate(origin _origin){
	switch (_origin){
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