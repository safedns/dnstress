#include <fcntl.h>

#include "servant.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"
#include "utils.h"

typedef void (*op_func)(evutil_socket_t, short, void *);

static struct servant_t *
get_servants(struct worker_t *worker, servant_type_t type)
{    
    switch (type) {
        case TCP_TYPE:
            return worker->tcp_servants;
            break;
        case UDP_TYPE:
            return worker->udp_servants;
            break;
        default:
            fatal("unknown servant type");
            break;
    }   
    return NULL;
}

static int
get_sock_type(servant_type_t type)
{
    switch (type) {
        case UDP_TYPE:
            return SOCK_DGRAM;
            break;
        case TCP_TYPE:
            return SOCK_STREAM;
            break;
        default:
            fatal("unknown servant type");
            break;
    }
    return -1;
}

static op_func
get_reply_callback(servant_type_t type)
{
    switch (type) {
        case TCP_TYPE:
            return recv_tcp_reply;
            break;
        case UDP_TYPE:
            return recv_udp_reply;
            break;
        default:
            fatal("unknown servant type");
            break;
    }
    return NULL;
}

void 
servant_init(struct worker_t *worker, size_t index, servant_type_t type)
{
    /* TODO: add error codes */
    if (worker == NULL) return;
    if (type == UDP_TYPE && (index < 0 || index >= worker->dnstress->max_udp_servants)) return;
    if (type == TCP_TYPE && (index < 0 || index >= worker->dnstress->max_tcp_servants)) return;
    if (type == CLEANED) return;
    
    int sock_type = get_sock_type(type);
    struct _saddr *sstor      = NULL;
    struct servant_t *servant = NULL;

    servant = &(get_servants(worker, type)[index]); /* TODO: make it simplier */
    sstor   = worker->server;

    servant->config = worker->config;
    servant->index  = index;
    servant->fd     = socket(sstor->addr.ss_family, sock_type, 0);
    servant->server = sstor;
    servant->type   = type;
    
    servant->worker_base = worker;

    servant->buffer = ldns_buffer_new(PKTSIZE);

    if (servant->buffer == NULL)
        fatal("failed to create ldns buffer");
    
    if (servant->fd < 0)
        fatal("failed to create a servant's socket");
        
    if (connect(servant->fd, (struct sockaddr *) &(sstor->addr), sstor->len) < 0) {
        log_info("%s: worker: %d | servant: %d | connection error --> %s", __func__, worker->index,
            servant->index, worker->server->repr);
        goto err_return;
    }        
    /* make non-blocking */
    long flags = fcntl(servant->fd, F_GETFL, 0);
    if (fcntl(servant->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        goto err_return;
    }

    if ((servant->ev_recv = event_new(worker->dnstress->evb, servant->fd, 
        EV_READ | EV_PERSIST, get_reply_callback(type), servant)) == NULL)
        fatal("failed to create servant's event");

    if (event_add(servant->ev_recv, NULL) == -1)
        fatal("failed to add servant's event");

    servant->active = true;
    return;

err_return:
    close(servant->fd);
    ldns_buffer_free(servant->buffer);
    servant->buffer = NULL;
    return;
}

void
servant_clear(struct servant_t *servant)
{
    close(servant->fd);
    event_free(servant->ev_recv);

    if (servant->buffer)
        ldns_buffer_free(servant->buffer);
    
    servant->config  = NULL;
    servant->server  = NULL;
    servant->ev_recv = NULL;
    servant->buffer  = NULL;
    
    servant->index  = 0;
    servant->fd     = -1;
    servant->type   = CLEANED;
}

void
tcp_servant_run(struct servant_t *servant)
{
    send_tcp_query(servant);
}

void
udp_servant_run(struct servant_t *servant)
{
    send_udp_query(servant);       
}

struct rstats_t *
gstats(struct servant_t *servant)
{
    if (servant == NULL)
        fatal("%s: null servant pointer", __func__);
    return servant->worker_base->dnstress->stats;
}