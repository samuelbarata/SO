#include "safe.h"

void* safe_malloc(size_t __size,origin _origin){
	void* p;
	p = malloc(__size);
	if(!p){
		perror("malloc failed");
		if(_origin)
			pthread_exit(NULL);
		else
			raise(SIGINT);
	}
	return p;
}