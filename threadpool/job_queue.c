//
// Created by an.pavlov on 11.10.15.
//

#include <pthread.h>
#include <stdlib.h>

#include "job_queue.h"

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











