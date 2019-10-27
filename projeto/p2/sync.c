/* Sistemas Operativos, DEI/IST/ULisboa 2019-20 */

#include "sync.h"
#include "lib/timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void sync_init(syncMech* sync){
    int ret = syncMech_init(sync, NULL);
    if(ret != 0){
        perror("sync_init failed\n");
        exit(EXIT_FAILURE);
    }
}

void sync_destroy(syncMech* sync){
    int ret = syncMech_destroy(sync);
    if(ret != 0){
        perror("sync_destroy failed\n");
        exit(EXIT_FAILURE);
    }
}

void sync_wrlock(syncMech* sync){
    int ret = syncMech_wrlock(sync);
    if(ret != 0){
        perror("sync_wrlock failed");
        exit(EXIT_FAILURE);
    }
}

void sync_rdlock(syncMech* sync){
    int ret = syncMech_rdlock(sync);
    if(ret != 0){
        perror("sync_rdlock failed");
        exit(EXIT_FAILURE);
    }
}

void sync_unlock(syncMech* sync){
    int ret = syncMech_unlock(sync);
    if(ret != 0){
        perror("sync_unlock failed");
        exit(EXIT_FAILURE);
    }
}

void mutex_init(pthread_mutex_t* mutex){
    #if defined (RWLOCK) || defined (MUTEX)
        int ret = pthread_mutex_init(mutex, NULL);
        if(ret != 0){
            perror("mutex_init failed\n");
            exit(EXIT_FAILURE);
        }
    #endif
}

void mutex_destroy(pthread_mutex_t* mutex){
    #if defined (RWLOCK) || defined (MUTEX)
        int ret = pthread_mutex_destroy(mutex);
        if(ret != 0){
            perror("mutex_destroy failed\n");
            exit(EXIT_FAILURE);
        }
    #endif
}

void mutex_lock(pthread_mutex_t* mutex){
    #if defined (RWLOCK) || defined (MUTEX)
        int ret = pthread_mutex_lock(mutex);
        if(ret != 0){
            perror("mutex_lock failed");
            exit(EXIT_FAILURE);
        }
    #endif
}

void mutex_unlock(pthread_mutex_t* mutex){
    #if defined (RWLOCK) || defined (MUTEX)
        int ret = pthread_mutex_unlock(mutex);
        if(ret != 0){
            perror("mutex_unlock failed");
            exit(EXIT_FAILURE);
        }
     #endif
}

void se_wait(sem_t* id){
    int ret = sem_wait(id);
    if(ret != 0){
        perror("sem_wait failed");
        exit(EXIT_FAILURE);
    }
}

int se_time_wait(sem_t* id){
    WAIT_TIME ts;
    ts.tv_sec=0;
    ts.tv_nsec=NSEC_OFFSET;
    int ret = sem_timedwait(id, &ts);
    if (ret == -1 && errno == ETIMEDOUT) //espera ts segundos; depois sai
        return 1;
    else if(ret != 0){
        perror("sem_timedwait failed");
        exit(EXIT_FAILURE);
    }
    return 0;

}

void se_post(sem_t* id){
    int ret = sem_post(id);
    if(ret != 0){
        perror("sem_post failed");
        exit(EXIT_FAILURE);
    }
}

void se_init(sem_t* id, unsigned int n){
    int ret = sem_init(id, 0, n);
    if(ret != 0){
        perror("sem_init failed");
        exit(EXIT_FAILURE);
    }
}

void se_destroy(sem_t* id){
    int ret = sem_destroy(id);
    if(ret != 0){
        perror("sem_init failed");
        exit(EXIT_FAILURE);
    }
}

int do_nothing(void* a){
    (void)a;
    return 0;
}
