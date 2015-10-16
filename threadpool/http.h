//
// Created by an.pavlov on 15.10.15.
//

#ifndef APGINX_HTTP_H
#define APGINX_HTTP_H

#include <event2/buffer.h>

void handle_request(struct evbuffer *buf, char *line);

#endif //APGINX_HTTP_H
