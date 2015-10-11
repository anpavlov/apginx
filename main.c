//
// Created by an.pavlov on 11.10.15.
//

//#include <iostream>
#include <errno.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <stdlib.h>
#include <string.h>

//using namespace std;

static void accept_connection_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* addr, int sock_len, void* arg);
static void accept_error_cb(struct evconnlistener* listener, void* arg);
static void echo_read_cb(struct bufferevent* buf_ev, void* arg);
static void echo_event_cb(struct bufferevent* buf_ev, short events, void* arg);


int main() {
    struct event_base* base;
    struct evconnlistener* listener;
    struct sockaddr_in sin;
    int port = 9090;

    base = event_base_new();
    if (!base) {
//        cout << "Error while creating event base" << endl;
        return -1;
    }

//    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    listener = evconnlistener_new_bind(base, accept_connection_cb, NULL, (LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE),
                                       -1, (struct sockaddr*) &sin, sizeof(sin));

    if (!listener) {
//        cout << "Error while creating listener" << endl;
        return -1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

//    cout << "starting dispatch" << endl;
    event_base_dispatch(base);
//    cout << "end end end" << endl;

    return 0;
}

static void accept_connection_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* addr, int sock_len, void* arg) {
    struct event_base* base = evconnlistener_get_base(listener);
    struct bufferevent* buf_ev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
//    cout << "connection callback" << endl;

    bufferevent_setcb(buf_ev, echo_read_cb, NULL, echo_event_cb, NULL);
    bufferevent_enable(buf_ev, (EV_READ|EV_WRITE));
}

static void accept_error_cb(struct evconnlistener* listener, void* arg) {
    struct event_base* base = evconnlistener_get_base(listener);
    int error = EVUTIL_SOCKET_ERROR();
//    cout << "Erorr " << error << " (" << evutil_socket_error_to_string(error) <<"). Quiting" << endl;
    event_base_loopexit(base, NULL);
}

static void echo_read_cb(struct bufferevent* buf_ev, void* arg) {
    struct evbuffer* buf_input = bufferevent_get_input(buf_ev);
    struct evbuffer* buf_output = bufferevent_get_output(buf_ev);

    evbuffer_add_buffer(buf_output, buf_input);
//    cout << "echo read callback" << endl;
}

static void echo_event_cb(struct bufferevent* buf_ev, short events, void* arg) {
    if (events & BEV_EVENT_ERROR)
        perror("Error bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
        bufferevent_free(buf_ev);

//    cout << "echo event callback" << endl;
}