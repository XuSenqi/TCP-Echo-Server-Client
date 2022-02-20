#ifndef __T_H__
#define __T_H__
 
#include <pthread.h>

typedef struct {
  pthread_t thread_tid; /* thread ID */
  long thread_count;    /* # connections handled */
} Thread;
Thread *tptr; /* array of Thread structures; calloc'ed */

#define MAXNCLI 32
int clifd[MAXNCLI], iget, iput;
pthread_mutex_t		clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		clifd_cond = PTHREAD_COND_INITIALIZER;

void thread_make(int i);
void Pthread_mutex_lock(pthread_mutex_t *mptr);
void Pthread_mutex_unlock(pthread_mutex_t *mptr);
void Pthread_cond_signal(pthread_cond_t *cptr);

#endif