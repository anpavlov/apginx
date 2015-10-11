//
// Created by an.pavlov on 11.10.15.
//

#ifndef APGINX_JOB_QUEUE_H
#define APGINX_JOB_QUEUE_H

#include "binary_semaphore.h"
#include "job.h"
#include "thread_pool.h"

typedef struct job_queue_ job_queue;

struct job_queue_ {
    pthread_mutex_t queue_mutex;
    job *front;
    job *back;
    bin_sem *has_jobs;
    int length;
};

// could pass queue instead of thread pool, not done for less work w/ **queue
int job_queue_init(thread_pool *thpool);
//void job_queue_clear(thread_pool* thpool);
void job_queue_push(thread_pool *thpool, job *newjob);
job* job_queue_pull(thread_pool *thpool);
//void job_queue_destroy(thread_pool* thpool);

#endif //APGINX_JOB_QUEUE_H
