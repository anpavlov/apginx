//
// Created by an.pavlov on 15.10.15.
//

#include "http.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

//#define DOC_ROOT "/home/orange/apginx_root"
//#define DOC_ROOT_LEN 24
#define DOC_INDEX "index.html"
#define DOC_INDEX_LEN 10

#define PARSE_LINE_RET_BAD -1
#define PARSE_LINE_RET_GET 0
#define PARSE_LINE_RET_HEAD 1
#define PARSE_LINE_RET_NOT_ALLOWED 2

typedef struct response_info {
    char *path;
    const char *type;
    char *doc_root;
    int doc_root_len;
    int method;
    int size;
} response_info;

static response_info *resp_init(char *line);
static void free_resp(response_info *resp);

static inline int is_hex(int x);
static size_t decode(char *url, size_t len, char *dest);
static const char *guess_type(char *path, size_t len);
static void file_info(response_info *resp);

static void add_headers(struct evbuffer *buf, char *code, const char *type, int len);
static void handle_get(response_info *resp, struct evbuffer *buf);
static void handle_head(response_info *resp, struct evbuffer *writebuf);

void handle_request(struct evbuffer *buf, char *line, char *doc_root, int doc_root_len) {
    response_info *resp = resp_init(line);
    if (resp->method == PARSE_LINE_RET_BAD) {
        add_headers(buf, "400 Bad Request", "text/html", 24);
        evbuffer_add_printf(buf, "<h1>400 Bad Request</h1>");
        free(resp);
        return;
    }

    resp->doc_root = doc_root;
    resp->doc_root_len = doc_root_len;

    file_info(resp);
    if (resp->size == -1) {
        add_headers(buf, "404 Not Found", "text/html", 22);
        evbuffer_add_printf(buf, "<h1>404 Not Found</h1>");
        free_resp(resp);
        return;
    } else if (resp->size == -2) {
        add_headers(buf, "403 Forbidden", "text/html", 22);
        evbuffer_add_printf(buf, "<h1>403 Forbidden</h1>");
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
            add_headers(buf, "405 Method Not Allowed", "text/html", 31);
            evbuffer_add_printf(buf, "<h1>405 Method Not Allowed</h1>");
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
    if (protocol == NULL || (strcmp(protocol, "HTTP/1.1") != 0 && strcmp(protocol, "HTTP/1.0"))) {
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

static inline int is_hex(int x) {
    return (x >= '0' && x <= '9') ||
           (x >= 'a' && x <= 'f') ||
           (x >= 'A' && x <= 'F');
}

static size_t decode(char *url, size_t len, char *dest) {
    char *out;
    char *end = url + len;
    int ch;

    for (out = dest; url <= end;) {
        ch = *url;
        if (ch == '+') {
            ch = ' ';
        } else if (ch == '%' && is_hex(*(url + 1)) && is_hex(*(url + 2))) {
            sscanf(url + 1, "%2x", &ch);
            url += 2;
        } else if (ch == '?') {
            *out = 0;
            return out - dest;
        }
        if (ch == '.' && (*(url + 1)) == '.' && (*(url + 2)) == '/') {
            url += 3;
        } else if (out) {
            *out = (char)ch;
            ++out;
            ++url;
        }
    }

//    if (out) {
//        *out = 0;
//    }
    return out - dest - 1;
}

static const char *guess_type(char *path, size_t len) {
    char *ending = path + len;
    if (strcmp(ending - 5, ".html") == 0) {
        return "text/html";
    } else if (strcmp(ending - 4, ".css") == 0) {
        return "text/css";
    } else if (strcmp(ending - 4, ".txt") == 0) {
        return "text/plain";
    } else if (strcmp(ending - 3, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(ending - 4, ".png") == 0) {
        return "image/png";
    } else if (strcmp(ending - 4, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(ending - 4, ".swf") == 0) {
        return "application/x-shockwave-flash";
    } else if (strcmp(ending - 4, ".jpg") == 0 || strcmp(ending - 5, ".jpeg") == 0) {
        return "image/jpeg";
    } else {
        return "application/octet-stream";
    }
}

// декод url (%, / в конце) + полный путь + ограничение по папке
static void file_info(response_info *resp) {
    int is_folder = 0;
    size_t len = strlen(resp->path);
    if (*(resp->path + len - 1) == '/') {
        is_folder = 1;
    }
    char *decoded = (char*)malloc(len + 1);
    len = decode(resp->path, len, decoded);

    len += resp->doc_root_len;
    if (is_folder == 1) {
        len += DOC_INDEX_LEN;
    }
    char *fullpath = (char*)malloc(len + 100);
    strcpy(fullpath, resp->doc_root);
    strcat(fullpath, decoded);
    if (is_folder == 1) {
        strcat(fullpath, DOC_INDEX);
    }
    free(decoded);

    resp->path = fullpath;

    struct stat s;
    if (stat(fullpath, &s) == 0) {
        resp->size = (int)(s.st_size);
        resp->type = guess_type(fullpath, len);
    } else if (is_folder == 1) {
        resp->size = -2;
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

static void add_headers(struct evbuffer *buf, char *code, const char *type, int len) {
    char date[32];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(date, 32, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    evbuffer_add_printf(buf,
                        "HTTP/1.1 %s\r\nServer: apginx/0.1\r\nConnection: close\r\nDate: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n",
                        code, date, len, type);
}

static void handle_get(response_info *resp, struct evbuffer *buf) {

    add_headers(buf, "200 OK", resp->type, resp->size);
    int fd = open(resp->path, O_RDONLY);
    evbuffer_add_file(buf, fd, 0, resp->size);

}

static void handle_head(response_info *resp, struct evbuffer *buf) {
    add_headers(buf, "200 OK", resp->type, resp->size);
}

