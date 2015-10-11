//
// Created by an.pavlov on 11.10.15.
//

#ifndef APGINX_BINARY_SEMAPHORE_H
#define APGINX_BINARY_SEMAPHORE_H

typedef struct bin_sem_ bin_sem;

void bin_sem_init(bin_sem *bsem); // should(?) be (sem, int) to init with 1 or 0
//void bin_sem_reset(bin_sem* bsem); // to 0
void bin_sem_post(bin_sem *bsem); // notify at least 1 thread
//void bin_sem_post_all(bin_sem* bsem); // notify all
void bin_sem_wait(bin_sem *bsem); // wait until val become 1

#endif //APGINX_BINARY_SEMAPHORE_H
