//
// Created by an.pavlov on 11.10.15.
//

#ifndef APGINX_THREAD_POOL_H
#define APGINX_THREAD_POOL_H

#include <pthread.h>
#include <event2/bufferevent.h>

typedef struct thread_ thread;
typedef struct thread_pool_ thread_pool;
typedef struct job_queue_ job_queue;

struct thread_pool_ {
    thread** threads;
    volatile int num_threads_alive;
//    volatile int num_threads_working;
    pthread_mutex_t thread_count_mutex;
    job_queue* queue;
};

thread_pool* thread_pool_init(int num_threads);
int thread_pool_add_work(thread_pool* thpool, struct bufferevent *incoming_bufev, int type);


#endif //APGINX_THREAD_POOL_H
