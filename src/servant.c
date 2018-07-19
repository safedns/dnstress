#include <fcntl.h>

#include "servant.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"
#include "utils.h"

typedef void (*op_func)(evutil_socket_t, short, void *);

static struct servant_t * get_servants(struct worker_t *worker, servant_type_t type) {
    if (type == TCP_TYPE)
        return worker->tcp_servants;
    else if (type == UDP_TYPE)
        return worker->udp_servants;
    else
        fatal("unknown servant type");
    return NULL;
}

static int get_sock_type(servant_type_t type) {
    if (type == UDP_TYPE)
        return SOCK_DGRAM;
    else if (type == TCP_TYPE)
        return SOCK_STREAM;
    else
        fatal("unknown servant type");
    return -1;
}

static op_func get_reply_callback(servant_type_t type) {
    if (type == TCP_TYPE)
        return recv_tcp_reply;
    else if (type == UDP_TYPE)
        return recv_udp_reply;
    else
        fatal("unknown servant type");
    return NULL;
}

void servant_init(struct worker_t *worker, size_t index, servant_type_t type) {
    int sock_type = get_sock_type(type);
    struct _saddr *sstor    = NULL;
    struct servant_t *servant = NULL;

    servant = &(get_servants(worker, type)[index]); /* TODO: make it simplier */
    sstor   = worker->server;

    servant->config = worker->config;
    servant->index  = index;
    servant->fd     = socket(sstor->addr.ss_family, sock_type, 0);
    
    if (servant->fd < 0)
        fatal("failed to create a servant's socket");
        
    if (connect(servant->fd, (struct sockaddr *) &(sstor->addr), sstor->len) < 0) {
        log_info("worker %d: connection error --> %s", worker->index, worker->server->repr);
        close(servant->fd);
        return;
    }        
    /* make non-blocking */
    long flags = fcntl(servant->fd, F_GETFL, 0);
    if (fcntl(servant->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(servant->fd);
        return;
    }

    if ((servant->ev_recv = event_new(worker->dnstress->evb, servant->fd, 
        EV_READ | EV_PERSIST, get_reply_callback(type), servant)) == NULL)
        fatal("failed to create servant's event");

    if (event_add(servant->ev_recv, NULL) == -1)
        fatal("failed to add servant's event");

    return;
}

void tcp_servant_run(struct servant_t *servant) {
    send_tcp_query(servant);
}

void udp_servant_run(struct servant_t *servant) {
    send_udp_query(servant);       
}