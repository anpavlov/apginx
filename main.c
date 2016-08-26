//
// Created by an.pavlov on 11.10.15.
//

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "threadpool/thread_pool.h"
#include "threadpool/job.h"

static const char *help_string = "Using: httpd [OPTION]...\n\n"
        "  -r\t\t\tDocument root DEFAULT: ./\n"
        "  -c\t\t\tAmount of workers (1-65535) DEFAULT: 4\n"
        "  -p\t\t\tPort number (1-65535) DEFAULT: 80\n"
        "  -h\t\t\tShow this message\n";

typedef struct arguments {
    int port;
    int ncpu;
    int is_help;
    char *doc_root;
} arguments;

static int init_server(arguments *args);
static arguments *parse_args(int argc, char **argv);
static void int_handler(int sig);

static void accept_connection_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* addr, int sock_len, void* thpool);
static void accept_error_cb(struct evconnlistener* listener, void* arg);

static struct event_base* base;

int main(int argc, char **argv) {
    arguments *args = parse_args(argc, argv);
    if (!args) {
        printf("Wrong options! Use -h to show help\n");
        return 0;
    }
    if (args->is_help == 1) {
        printf("%s", help_string);
        return 0;
    }
    if (args->port < 1 || args->port > 65535) {
        printf("Wrong port number! Use -h to show help\n");
        return 0;
    }
    if (args->ncpu < 1 || args->ncpu > 65535) {
        printf("Wrong cpu amount! Use -h to show help\n");
        return 0;
    }

    pthread_t t1, t2;
    pthread_create(&t1, NULL, (void*)init_server, args);
    pthread_join(t1, NULL);
//    pthread_create(&t2, NULL, (void*)init_server, args);
//    pthread_join(t2, NULL);
//    init_server(args);
    return 0;
}

static arguments *parse_args(int argc, char **argv) {
    arguments *args = (arguments*)malloc(sizeof(arguments));
    args->port = 8080;
    args->ncpu = 4;
    args->doc_root = ".";
    args->is_help = 0;

    opterr = 0;
    int res = 0;
    while ((res = getopt(argc, argv, "p:c:r:h")) != -1) {
        switch (res) {
            case 'p':
                args->port = atoi(optarg);
                break;
            case 'c':
                args->ncpu = atoi(optarg);
                break;
            case 'r':
                args->doc_root = optarg;
                if (args->doc_root[strlen(args->doc_root) - 1] == '/') {
                    args->doc_root[strlen(args->doc_root) - 1] = 0;
                }
                break;
            case 'h':
                args->is_help = 1;
                return args;
            default:
                free(args);
                return NULL;
        }
    }
    return args;
}

static int init_server(arguments *args) {
    printf("Starting server...\n");

    if (evthread_use_pthreads() == -1) {
        printf("Error occured while turning on pthreads using\n");
        return -1;
    }
    signal(SIGINT, int_handler);

    struct evconnlistener* listener;
    struct sockaddr_in sin;

    base = event_base_new();
    if (!base) {
        printf("Error while creating event base\n");
        free(args);
        return -1;
    }

    thread_pool* thpool = thread_pool_init(args->ncpu, args->doc_root);
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons((unsigned short)args->port);

    listener = evconnlistener_new_bind(base, accept_connection_cb, (void*)thpool, (LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE),
                                       -1, (struct sockaddr*) &sin, sizeof(sin));

    free(args);

    if (!listener) {
        printf("Error while creating listener\n");
        thread_pool_destroy(thpool);
        event_base_free(base);
        return -1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

    printf("Server is running\nUse Ctrl+C to stop server\n");
    event_base_dispatch(base);

    evconnlistener_free(listener);
    thread_pool_destroy(thpool);
    event_base_free(base);
    return 0;
}

static void accept_connection_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* addr, int sock_len, void* thpool) {
    struct event_base* base = evconnlistener_get_base(listener);
    struct bufferevent* buf_ev = bufferevent_socket_new(base, fd, (BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE));

    thread_pool_add_work(thpool, buf_ev, JOB_TYPE_NEW);
}

static void accept_error_cb(struct evconnlistener* listener, void* arg) {
    struct event_base* base = evconnlistener_get_base(listener);
    int error = EVUTIL_SOCKET_ERROR();
    printf("Error %d (%s). Shutting down...", error, evutil_socket_error_to_string(error));
    event_base_loopexit(base, NULL);
}

static void int_handler(int sig) {
    printf("\nShutting down...\n");
    event_base_loopexit(base, NULL);
}
