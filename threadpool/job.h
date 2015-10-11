//
// Created by an.pavlov on 11.10.15.
//

#ifndef APGINX_JOB_H
#define APGINX_JOB_H

#include <event2/buffer.h>

typedef struct job_ job;

struct job_ {
    job *prev;
    struct evbuffer *inputbuf;
    struct evbuffer *outputbuf;
//    some info (socket, buffer)
//TODO: job fields
};

#endif //APGINX_JOB_H
