//
// Created by an.pavlov on 11.10.15.
//

#ifndef APGINX_JOB_H
#define APGINX_JOB_H

#define JOB_TYPE_NEW 0
#define JOB_TYPE_DATA 1

#include <event2/bufferevent.h>

typedef struct job_ job;

struct job_ {
    job *prev;
    int type;
    struct bufferevent *bufev;
//    struct evbuffer *inputbuf;
//    some info (socket, buffer)
};

#endif //APGINX_JOB_H
