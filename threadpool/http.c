//
// Created by an.pavlov on 15.10.15.
//

// TODO: headers:
// TODO:    Server
// TODO:    Connection
// TODO:    Date
// TODO: parse % and + and ../
// TODO: add index.html to */

#include "http.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#define DOC_ROOT "/home/orange/apginx_root"
#define DOC_ROOT_LEN 24

#define PARSE_LINE_RET_BAD -1
#define PARSE_LINE_RET_GET 0
#define PARSE_LINE_RET_HEAD 1
#define PARSE_LINE_RET_NOT_ALLOWED 2

typedef struct response_info {
    char *path;
    int method;
    int size;
} response_info;

static response_info *resp_init(char *line);
static void file_info(response_info *resp);
static void free_resp(response_info *resp);
static void handle_get(response_info *resp, struct evbuffer *writebuf);
static void handle_head(response_info *resp, struct evbuffer *writebuf);

void handle_request(struct evbuffer *buf, char *line) {
    response_info *resp = resp_init(line);
    if (resp->method == PARSE_LINE_RET_BAD) {
        evbuffer_add_printf(buf, "HTTP/1.1 400 Bad Request\r\n");
//        TODO: add header and probably content
        free(resp);
        return;
    }

    file_info(resp);
    if (resp->size == -1) {
        evbuffer_add_printf(buf, "HTTP/1.1 404 Not Found\r\n");
//        TODO: add header and probably content
        free_resp(resp);
        return;
    }

    switch (resp->method) {
        case PARSE_LINE_RET_GET:
            handle_get(resp, buf);
            break;
        case PARSE_LINE_RET_HEAD:
            handle_head(resp, buf);
            break;
        case PARSE_LINE_RET_NOT_ALLOWED:
            evbuffer_add_printf(buf, "HTTP/1.1 405 Method Not Allowed\r\n");
//        TODO: add header and probably content
            free_resp(resp);
            return;
        default:
//            fucked up
            break;
    }
    free_resp(resp);
}

static response_info *resp_init(char *line) {
    response_info *resp = (response_info*)malloc(sizeof(response_info));
    resp->path = NULL;

//    split first line by whitespaces
    char *method = strtok(line, " ");
    if (method == NULL) {
        resp->method = PARSE_LINE_RET_BAD;
        return resp;
    }

    char *incoming_path = strtok(NULL, " ");
    if (incoming_path == NULL) {
        resp->method = PARSE_LINE_RET_BAD;
        return resp;
    }

    char *protocol = strtok(NULL, " ");
    if (protocol == NULL || strcmp(protocol, "HTTP/1.1") != 0) {
        resp->method = PARSE_LINE_RET_BAD;
        return resp;
    }

    resp->path = incoming_path;

    if (strcmp(method, "GET") == 0) {
        resp->method = PARSE_LINE_RET_GET;
    } else if (strcmp(method, "HEAD") == 0) {
        resp->method = PARSE_LINE_RET_HEAD;
    } else {
        resp->method = PARSE_LINE_RET_NOT_ALLOWED;
    }

    return resp;
}

// декод url (%, / в конце) + полный путь + ограничение по папке
static void file_info(response_info *resp) {
//    TODO: decode url

    size_t len = strlen(resp->path);
    len += DOC_ROOT_LEN;
    char *fullpath = (char*)malloc(len);
    strcat(fullpath, DOC_ROOT);
    strcat(fullpath, resp->path);
    resp->path = fullpath;
    struct stat s;
    if (stat(fullpath, &s) == 0) {
        resp->size = (int)(s.st_size);
    } else {
        resp->size = -1;
    }
}

static void free_resp(response_info *resp) {
    if (resp->path) {
        free(resp->path);
    }
    free(resp);
}

static void handle_get(response_info *resp, struct evbuffer *writebuf) {

    int fd = open(resp->path, O_RDONLY);
    evbuffer_add_printf(writebuf, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", resp->size);
    evbuffer_add_file(writebuf, fd, 0, resp->size);

}

static void handle_head(response_info *resp, struct evbuffer *writebuf) {

}

