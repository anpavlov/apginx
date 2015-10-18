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

#include "threadpool/thread_pool.h"
#include "threadpool/job.h"

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
//       TODO: print wrong parameters and suggest help
        return 0;
    }
    if (args->is_help == 1) {
//       TODO: print help
        return 0;
    }
    if (args->port < 1 || args->port > 65535) {
//        TODO: print error
        return 0;
    }
    if (args->ncpu < 1 || args->ncpu > 65535) {
//        TODO: print error
        return 0;
    }
    return init_server(args);
}

static arguments *parse_args(int argc, char **argv) {
    arguments *args = (arguments*)malloc(sizeof(arguments));
    args->port = 80;
    args->ncpu = 4;
    args->doc_root = ".";
    args->is_help = 0;

    opterr = 0;
    int res = 0;
    while ((res = getopt(argc, argv, "p:c:r:")) != -1) {
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
                break;
            default:
                free(args);
                return NULL;
        }
    }
    return args;
}

static int init_server(arguments *args) {
    if (evthread_use_pthreads() == -1) {
//        TODO: print error
        return -1;
    }
    signal(SIGINT, int_handler);

    struct evconnlistener* listener;
    struct sockaddr_in sin;

    base = event_base_new();
    if (!base) {
//        cout << "Error while creating event base" << endl;
//        TODO: print error
        return -1;
    }

    thread_pool* thpool = thread_pool_init(args->ncpu, args->doc_root);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(args->port);

    listener = evconnlistener_new_bind(base, accept_connection_cb, (void*)thpool, (LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE),
                                       100, (struct sockaddr*) &sin, sizeof(sin));

    if (!listener) {
//        cout << "Error while creating listener" << endl;
//        TODO: print error
        return -1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);


    event_base_dispatch(base);
//    printf("now back to main\n");
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
//    int error = EVUTIL_SOCKET_ERROR();
//    cout << "Erorr " << error << " (" << evutil_socket_error_to_string(error) <<"). Quiting" << endl;
    event_base_loopexit(base, NULL);
}

static void int_handler(int sig) {
//    printf("i'm a SIGINT handler!\n");
    event_base_loopexit(base, NULL);
//    printf("loop's broken!\n");
}
