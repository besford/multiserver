#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "threadpool.h"

//task waiting execution
typedef struct task_t {
    void (*routine)(void *);
    void *args;
    struct task_t *next;
} task_t;

//queue of tasks to be scheduled to threads
typedef struct taskqueue_t {
    pthread_cond_t empty;
    pthread_cond_t notempty;
    task_t *head;
    task_t *tail;
    int pending;
    int reject;
} taskqueue_t;

//the threadpool itself
struct threadpool_t {
    pthread_mutex_t lock;
    pthread_t *threads;
    taskqueue_t *tasks;
    int threads_num;
    int threads_running;
    int shutdown;
};

//Worker thread of for threadpool threads
void *worker(void *p) {
    //get worker's pool
    threadpool_t *pool = (threadpool_t *)p;

    //main loop for worker
    while (1) {
        //lock tasks
        pthread_mutex_lock(&pool->lock);

        //wait until task has been scheduled
        while (!pool->tasks->pending) {
            //unlock pool and wait for tasks
            pthread_mutex_unlock(&pool->lock);
            pthread_cond_wait(&pool->tasks->notempty, &pool->lock);

            //shutting down, terminate thread
            if (pool->shutdown == 1) {
                pthread_mutex_unlock(&pool->lock);
                return NULL;
            }
        }

        //pop current task off of queue to be bandled by worker
        task_t *cur = pool->tasks->head;
        pool->tasks->pending--;

        //if pool is now empty, set null
        //else update head ofg pool
        if (!pool->tasks->pending)
            pool->tasks->head = pool->tasks->tail = NULL;
        else
            pool->tasks->head = cur->next;

        //notify pool that all pending tasks are done
        if (!pool->tasks->pending)
            pthread_cond_signal(&pool->tasks->empty); //wake workers

        //notify pool that not shutting down
        if (!pool->shutdown)
            pthread_cond_signal(&pool->tasks->empty); //wake workers

        //unlock pool
        pthread_mutex_unlock(&pool->lock);

        //handle scheduled task
        (cur->routine)(cur->args);

        //destroy current task
        free(cur);
    }

    //to appease the compiler
    return NULL;
}

//Initalizes the thread pool
threadpool_t *threadpool_create(int num_threads, int num_tasks) {
    threadpool_t *pool;

    //check if valid number of threads
    if (num_threads <= 0 || num_threads > MAX_THREADS)
        return NULL;

    //allocate memory for pool
    pool = (threadpool_t *) malloc(sizeof(threadpool_t));
    if (pool == NULL)
        return NULL;

    //allocate memory for threads
    pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);
    if (pool->threads == NULL)
        return NULL;

    //allocate memory for tasks queue
    pool->tasks = (taskqueue_t *) malloc(sizeof(taskqueue_t));
    if (pool->tasks == NULL) {
        threadpool_destroy(pool);
        return NULL;
    }

    //initialize empty condition flag
    //initialize nonempty condition flag
    int notempty = pthread_cond_init(&pool->tasks->notempty, NULL);
    int empty = pthread_cond_init(&pool->tasks->empty, NULL);
    if (empty != 0 || notempty != 0) {
        threadpool_destroy(pool);
        return NULL;
    }

    //initialize pool lock
    int lock = pthread_mutex_init(&pool->lock, NULL);
    if (lock != 0) {
        threadpool_destroy(pool);
        return NULL;
    }

    //initialize tasks queue
    pool->tasks->pending = 0;
    pool->tasks->head = NULL;
    pool->tasks->tail = NULL;
    pool->tasks->reject = 0;
    pool->shutdown = 0;

    //start worker threads
    for (int i = 0; i < num_threads; i++) {
        //create worker
        int created = pthread_create(&pool->threads[i], NULL, worker, pool);
        printf("---worker %i created\n", i);

        //check if successfully created
        if (created != 0) {
            threadpool_destroy(pool);
            return NULL;
        }

        //increment theeads running
        pool->threads_running++;
    }

    pool->threads_num = pool->threads_running;

    return pool;
}

//schedules task to available worker threads
//else returns null
int threadpool_schedule(threadpool_t *pool, task_fn function, void *args) {
    //sanity check
    if (pool == NULL)
        return -1;

    //make sure that tasks queue does not exceed limit
    if (pool->tasks->pending + 1 > MAX_TASKS)
        return -2;

    //allocate memory for new tasks
    task_t *new_task = (task_t *)malloc(sizeof(task_t));
    if (new_task == NULL)
        return -3;

    //initialize new task
    new_task->routine = function;
    new_task->args = args;
    new_task->next = NULL;

    //lock task queue
    pthread_mutex_lock(&pool->lock);

    //if not rejecting, queue task
    //else return error and destroy task
    if (!pool->tasks->reject) {
        //if tasks not pending, task is only task. Update queue to not empty and announce to threads
        //if tasks pending, append to end of queue
        if (!pool->tasks->pending) {
            pool->tasks->head = pool->tasks->tail = new_task;
            pthread_cond_signal(&pool->tasks->notempty);
        }
        else {
            pool->tasks->tail->next = new_task;
            pool->tasks->tail = new_task;
        }

        //incremenent pending tasks
        pool->tasks->pending++;

        //unlock queue
        pthread_mutex_unlock(&pool->lock);

        return 0;
    }
    else {
        free(new_task);
        return -2;
    }
}

//destroys the tasks queue, including threads, flags, and mutex locks
//else returns error
int threadpool_destroy_tasks(threadpool_t *pool) {
    //sanity check. Don't destroy if threads active
    if (pool == NULL || pool->threads_running > 0)
        return -1;

    //if threads created, destroy threads
    if (pool->threads != NULL)
        free(pool->threads);

    //destroy flags and lock
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->tasks->empty);
    pthread_cond_destroy(&pool->tasks->notempty);

    //destroy tasks queue
    free(pool->tasks);
    free(pool);

    return 0;
}

//destroys the thread pool
int threadpool_destroy(threadpool_t *pool) {
    //lock pool
    pthread_mutex_lock(&pool->lock);

    //reject new tasks, wait until all tasks have finished
    pool->tasks->reject = 1;
    while (pool->tasks->pending > 0)
        pthread_cond_wait(&pool->tasks->empty, &pool->lock);

    //set shutdown flag, broadcast queue notempty
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->tasks->notempty); //wake workers

    //unlock pool
    pthread_mutex_unlock(&pool->lock);

    //kill worker threads
    for (int i = 0; i < pool->threads_num; i++)
    {
        printf("---worker %i killed\n", i);
        pthread_join(pool->threads[i], NULL);           //kill worker when done task
        pthread_cond_broadcast(&pool->tasks->notempty); //stop threads from sleeping
        pool->threads_running--;
    }

    //deallocate pool
    threadpool_destroy_tasks(pool);

    return 0;
}
