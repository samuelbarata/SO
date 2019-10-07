#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

#define nothing do {} while (0) //linha vai ser apagada pelo compilador pq nao faz nada

#ifdef DMUTEX
    #define THREADS

    #define unlock_mutex(lock) pthread_mutex_unlock(lock)
    #define lock_mutex(lock) pthread_mutex_lock(lock)

    #define lock_rw(lock) nothing
    #define lock_r(lock) nothing
    #define unlock_rw(lock) nothing
   
#elif RWLOCK
    #define THREADS

    #define unlock_mutex(lock) nothing
    #define lock_mutex(lock) nothing

    #define lock_rw(lock) pthread_rwlock_wrlock(lock)
    #define lock_r(lock) pthread_rwlock_rdlock(lock)
    #define unlock_rw(lock) pthread_rwlock_unlock(lock)

#else
    #define unlock_mutex(lock) nothing
    #define lock_mutex(lock) nothing

    #define lock_rw(lock) nothing
    #define lock_r(lock) nothing
    #define unlock_rw(lock) nothing
#endif