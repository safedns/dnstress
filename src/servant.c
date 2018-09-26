#include <fcntl.h>

#include "servant.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"
#include "utils.h"

#define SERV_TIMEOUT 5

typedef void (*op_func)(evutil_socket_t, short, void *);

static struct servant_t *
get_servants(const struct worker_t *worker,
    const servant_type_t type)
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
get_sock_type(const servant_type_t type)
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
get_reply_callback(const servant_type_t type)
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

static void
servant_timeout_cb(evutil_socket_t fd, short events, void *arg)
{
    struct servant_t *servant = (struct servant_t *) arg;
    /* TODO: Implement servant's timeout event */
    return;
}

int
servant_init(struct worker_t *worker, const size_t index,
    const servant_type_t type)
{
    /* TODO: add error codes */
    if (worker == NULL)
        return NULL_WORKER;
    if (type == UDP_TYPE && (index < 0 || index >= worker->dnstress->max_udp_servants))
        return INCORRECT_INDEX;
    if (type == TCP_TYPE && (index < 0 || index >= worker->dnstress->max_tcp_servants))
        return INCORRECT_INDEX;
    if (type == CLEANED)
        return CLEANED_ERROR;
    
    struct timeval tv;
    // int set = 1;

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

    servant->buffer = ldns_buffer_new(LDNS_MAX_PACKETLEN);

    if (servant->fd < 0)
        fatal("%s: failed to create a servant's socket", __func__);

    if (servant->buffer == NULL)
        fatal("%s: failed to create ldns buffer", __func__);

#ifdef SO_NOSIGPIPE
    setsockopt(servant->fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
        
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
    
    if ((servant->ev_timeout = event_new(worker->dnstress->evb, -1, 0,
        servant_timeout_cb, servant)))
    
    timerclear(&tv);
    tv.tv_sec = SERV_TIMEOUT;
    if (event_add(servant->ev_timeout, &tv) == -1)
        fatal("failed to add servant's timeout event");
    
    if (event_add(servant->ev_recv, NULL) == -1)
        fatal("failed to add servant's recv event");

    servant->active = true;
    
    return 0;

err_return:
    close(servant->fd);
    ldns_buffer_free(servant->buffer);
    servant->buffer = NULL;
    
    return CREATE_ERROR;
}

int
servant_clear(struct servant_t *servant)
{
    if (servant == NULL)
        return SERVANT_NULL;

    close(servant->fd);
    event_free(servant->ev_recv);

    if (servant->buffer != NULL)
        ldns_buffer_free(servant->buffer);
    
    servant->config  = NULL;
    servant->server  = NULL;
    servant->ev_recv = NULL;
    servant->buffer  = NULL;
    
    servant->index  = 0;
    servant->fd     = -1;
    servant->type   = CLEANED;

    servant->active = false;

    return 0;
}

void
tcp_servant_run(const struct servant_t *servant)
{
    send_tcp_query(servant);
}

void
udp_servant_run(const struct servant_t *servant)
{
    send_udp_query(servant);       
}

struct rstats_t *
gstats(const struct servant_t *servant)
{
    if (servant == NULL)
        fatal("%s: null pointer to servant", __func__);
    
    if (servant->worker_base == NULL)
        fatal("%s: null pointer to worker base", __func__);
    
    if (servant->worker_base->dnstress == NULL)
        fatal("%s: null pointer to dnstress", __func__);
    
    if (servant->worker_base->dnstress->stats == NULL)
        fatal("%s: null pointer to stats", __func__);
    
    return servant->worker_base->dnstress->stats;
}