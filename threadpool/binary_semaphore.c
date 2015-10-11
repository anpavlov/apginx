//
// Created by an.pavlov on 11.10.15.
//

#include <pthread.h>

#include "binary_semaphore.h"

struct bin_sem_ {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int value;
};

//void bin_sem_init(bin_sem* bsem); // should(?) be (sem, int) to init with 1 or 0
////void bin_sem_reset(bin_sem* bsem); // to 0
//void bin_sem_post(bin_sem* bsem); // notify at least 1 thread
////void bin_sem_post_all(bin_sem* bsem); // notify all
//void bin_sem_wait(bin_sem* bsem); // wait until val become 1

void bin_sem_init(bin_sem *bsem) {
    pthread_mutex_init(&bsem->mutex, NULL);
    pthread_cond_init(&bsem->condition, NULL);
    bsem->value = 0;
}

void bin_sem_post(bin_sem *bsem) {
    pthread_mutex_lock(&bsem->mutex);
    bsem->value = 1;
    pthread_cond_signal(&bsem->condition);
    pthread_mutex_unlock(&bsem->mutex);
}

void bin_sem_wait(bin_sem *bsem) {
    pthread_mutex_lock(&bsem->mutex);
    while (bsem->value != 1) {
        pthread_cond_wait(&bsem->condition, &bsem->mutex);
    }
    bsem->value = 0;
    pthread_mutex_unlock(&bsem->mutex);
}