/*
    definer.h
    Inclui os defines originais do main.c
    Inclui as funcoes dos locks para cada uma das flags
*/

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

//ignorada pelo compilador (gcc)
#define nothing do {} while (0)

#ifdef MUTEX
    #define unlock_mutex(lock) pthread_mutex_unlock(lock)
    #define lock_mutex(lock) pthread_mutex_lock(lock)
#else
    #define unlock_mutex(lock) nothing
    #define lock_mutex(lock) nothing
#endif

#ifdef RWLOCK
    #define lock_rw(lock) pthread_rwlock_wrlock(lock)
    #define lock_r(lock) pthread_rwlock_rdlock(lock)
    #define unlock_rw(lock) pthread_rwlock_unlock(lock)
#else
    #define lock_rw(lock) nothing
    #define lock_r(lock) nothing
    #define unlock_rw(lock) nothing
#endif
