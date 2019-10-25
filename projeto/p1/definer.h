/*
    definer.h
    Inclui os defines originais do main.c
    Inclui as funcoes dos locks para cada uma das flags
*/

#ifndef DEFINER
#define DEFINER
#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads, numberBuckets;

//ignorada pelo compilador (gcc)
#define nothing do {} while (0)

#ifdef MUTEX
    #define syncMech            pthread_mutex_t
    #define sync_unlock(lock)   pthread_mutex_unlock(lock)
    #define sync_rw_lock(lock)  pthread_mutex_lock(lock)
    #define sync_r_lock(lock)   pthread_mutex_lock(lock)
    #define sync_init(lock, b)  pthread_mutex_init(lock, b)
    #define sync_destroy(lock)  pthread_mutex_destroy(lock)
#elif RWLOCK
    #define syncMech            pthread_rwlock_t
    #define sync_rw_lock(lock)  pthread_rwlock_wrlock(lock)
    #define sync_r_lock(lock)   pthread_rwlock_rdlock(lock)
    #define sync_unlock(lock)   pthread_rwlock_unlock(lock)
    #define sync_init(lock, b)  pthread_rwlock_init(lock, b)
    #define sync_destroy(lock)  pthread_rwlock_destroy(lock)
#else
    #define syncMech            void*
    #define sync_rw_lock(lock)  nothing
    #define sync_r_lock(lock)   nothing
    #define sync_unlock(lock)   nothing
    #define sync_init(lock, b)  nothing
    #define sync_destroy(lock)  nothing
#endif
#endif
