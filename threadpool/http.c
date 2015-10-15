//
// Created by an.pavlov on 15.10.15.
//

#include "http.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

static int write_file(char *filepath, struct evbuffer *writebuf);
static int write_header(char *filepath, struct evbuffer *writebuf);

void handle_request(struct evbuffer *buf, char *line) {
    //                        split first line by whitespaces
    char *splited[3];
    int count = 0;
    char *tok = strtok(line, " ");
    while (tok != NULL) {
        splited[count] = tok;
        tok = strtok(NULL, " ");
        ++count;
//                            no need to see more than 3 first tokens
        if (count > 2) {
            break;
        }
    }
    if (count < 2) {
        evbuffer_add_printf(buf, "\n");
    } else {

        if (strcmp(splited[2], "HTTP/1.1") == 0) {
            if (strcmp(splited[0], "GET") == 0) {
                int ret = write_file(splited[1], buf);
//                                if (ret != 0) {
//                                    evbuffer_add_printf(buf_output, "\n");
//                                }
            } else if (strcmp(splited[0], "HEAD") == 0) {
                write_header(splited[1], buf);
            } else {
//                                method not supported
            }
        } else {
            evbuffer_add_printf(buf, "\n");
        }
    }
}


static int write_file(char *filepath, struct evbuffer *writebuf) {
    size_t len = strlen(filepath);
    len += DOC_ROOT_LEN;
    char *fullpath = (char*)malloc(len);
    strcat(fullpath, DOC_ROOT);
    strcat(fullpath, filepath);

    int fd = open(fullpath, O_RDONLY);
    if (fd == -1) {
//        return 404
        evbuffer_add_printf(writebuf, "HTTP/1.1 404 Not Found");
    } else {

        evbuffer_add_printf(writebuf, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 57\r\n\r\n");

        struct stat st;
        stat(fullpath, &st);

        evbuffer_add_file(writebuf, fd, 0, st.st_size);
    }

    free(fullpath);
}

static int write_header(char *filepath, struct evbuffer *writebuf) {

}