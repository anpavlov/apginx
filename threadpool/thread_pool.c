//
// Created by an.pavlov on 11.10.15.
//

#include "job.h"
#include "binary_semaphore.h"
#include <stdlib.h>

#include "thread_pool.h"

struct thread_ {
//    int id;
    pthread_t pthread;
    thread_pool *thpool;
};

struct job_queue_ {
    pthread_mutex_t queue_mutex;
    job *front;
    job *back;
    bin_sem *has_jobs;
    int length;
};

static int threads_alive;

static int thread_init(thread_pool *thpool, int id);
static void thread_do(thread *thrd);

// could pass queue instead of thread pool, not done for less work w/ **queue
static int job_queue_init(thread_pool *thpool);
//void job_queue_clear(thread_pool* thpool);
static void job_queue_push(thread_pool *thpool, job *newjob);
static job* job_queue_pull(thread_pool *thpool);
//void job_queue_destroy(thread_pool* thpool);


// =========== THREAD POOL ==============

thread_pool *thread_pool_init(int num_threads) {
    threads_alive = 1;

    if (num_threads < 0) {
        num_threads = 0;
    }

    thread_pool *thpool;
    thpool = (thread_pool*)malloc(sizeof(thread_pool));
    if (!thpool) {
//        cant allocate
        return NULL;
    }

    thpool->num_threads_alive = 0;

    if (job_queue_init(thpool) != 0) {
//        cant allocate for queue
        free(thpool);
        return NULL;
    }

    thpool->threads = (thread**)malloc(num_threads * sizeof(thread));
    if (!thpool->threads) {
//        cant allocate for array
//        job_queue_destroy(thpool);
//        free(thpool->queue);
        free(thpool);
    }

    pthread_mutex_init(&thpool->thread_count_mutex, NULL);

    int i, ret;
    for (i = 0; i < num_threads; ++i) {
        ret = thread_init(thpool, i);
        if (ret != 0) {
//            cant allocate
            return NULL;
        }
    }

    while (thpool->num_threads_alive != num_threads) {}

    return thpool;
}

int thread_pool_add_work(thread_pool *thpool, struct evbuffer *input, struct evbuffer *output) {
    job *newjob;
    newjob = (job*)malloc(sizeof(job));
    if (!newjob) {
//        cant allocate
        return -1;
    }

    newjob->inputbuf = input;
    newjob->outputbuf = output;

    pthread_mutex_lock(&thpool->queue->queue_mutex);
    job_queue_push(thpool, newjob);
    pthread_mutex_unlock(&thpool->queue->queue_mutex);

    return 0;
}

// =========== THREADS ==============

int thread_init(thread_pool *thpool, int id) {
    thread *curthread = (thread*)malloc(sizeof(thread));
    if (!curthread) {
//        cant allocate
        return -1;
    }
    thpool->threads[id] = curthread;

    curthread->thpool = thpool;

    pthread_create(&curthread->pthread, NULL, (void*)thread_do, curthread);
    pthread_detach(curthread->pthread);

    return 0;
}

void thread_do(thread *thrd) {
    thread_pool* thpool = thrd->thpool;

    pthread_mutex_lock(&thpool->thread_count_mutex);
    ++thpool->num_threads_alive;
    pthread_mutex_unlock(&thpool->thread_count_mutex);

    while (threads_alive) {
        bin_sem_wait(thpool->queue->has_jobs);

        if (threads_alive) {
            job* curjob;
            pthread_mutex_lock(&thpool->queue->queue_mutex);
            curjob = job_queue_pull(thpool);
            pthread_mutex_unlock(&thpool->queue->queue_mutex);
            if (curjob) {

                evbuffer_add_buffer(curjob->outputbuf, curjob->inputbuf);
//                TODO: do some work with job
                free(curjob);
            }
        }
    }

    pthread_mutex_lock(&thpool->thread_count_mutex);
    --thpool->num_threads_alive;
    pthread_mutex_unlock(&thpool->thread_count_mutex);
}

// =========== JOB QUEUE ==============

int job_queue_init(thread_pool *thpool) {
    thpool->queue = (job_queue*)malloc(sizeof(job_queue));
    if (!thpool->queue) {
        return -1;
    }

    thpool->queue->length = 0;
    thpool->queue->front = NULL;
    thpool->queue->back = NULL;

    pthread_mutex_init(&thpool->queue->queue_mutex, NULL);

    thpool->queue->has_jobs = (bin_sem*)malloc(sizeof(bin_sem));
    if (!thpool->queue->has_jobs) {
        return -1;
    }

    bin_sem_init(thpool->queue->has_jobs);

    return 0;
}

void job_queue_push(thread_pool *thpool, job *newjob) {
    newjob->prev = NULL;

    if (thpool->queue->length == 0) {
        thpool->queue->front = newjob;
        thpool->queue->back = newjob;
    } else { // queue not empty
        thpool->queue->back->prev = newjob;
        thpool->queue->back = newjob;
    }

    ++thpool->queue->length;

    bin_sem_post(thpool->queue->has_jobs);
}

job *job_queue_pull(thread_pool *thpool) {
    job* pulledjob;
    pulledjob = thpool->queue->front;

    switch (thpool->queue->length) {
        case 0:
            break;

        case 1:
            thpool->queue->front = NULL;
            thpool->queue->back = NULL;
            thpool->queue->length = 0;
            break;

        default:
            thpool->queue->front = pulledjob->prev;
            --thpool->queue->length;
//            some jobs left in queue - post it
            bin_sem_post(thpool->queue->has_jobs);
            break;
    }

    return pulledjob;
}