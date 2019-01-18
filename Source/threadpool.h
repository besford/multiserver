#ifndef THREADPOOL_H
#define THREADPOOL_H

#define MAX_THREADS   64
#define MAX_TASKS     65536

typedef struct threadpool_t threadpool_t;
typedef void (*task_fn)(void *);

threadpool_t* threadpool_create(int num_threads, int num_tasks);
int           threadpool_schedule(threadpool_t* pool, task_fn, void *arg);
int           threadpool_destroy(threadpool_t* pool);

#endif
