#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "worker.h"
#include "utils.h"
#include "udp.h"
#include "tcp.h"

typedef void (*op_func)(evutil_socket_t, short, void *);

static int get_sock_type(sender_type_t type) {
    if (type == UDP_TYPE)
        return SOCK_DGRAM;
    else if (type == TCP_TYPE)
        return SOCK_STREAM;
    else
        fatal("[-] Unknown sender type");
    return -1;
}

static struct sender_t * get_senders(struct worker_t *worker, sender_type_t type) {
    if (type == TCP_TYPE)
        return worker->tcp_senders;
    else if (type == UDP_TYPE)
        return worker->udp_senders;
    else
        fatal("[-] Unknown sender type");
    return NULL;
}

static op_func get_reply_callback(sender_type_t type) {
    if (type == TCP_TYPE)
        return recv_tcp_reply;
    else if (type == UDP_TYPE)
        return recv_udp_reply;
    else
        fatal("[-] Unknown sender type");
    return NULL;
}

static void sender_init(struct worker_t *worker, size_t index, sender_type_t type) {
    int sock_type = get_sock_type(type);
    struct _saddr *sstor    = NULL;
    struct sender_t *sender = NULL;

    sender = &(get_senders(worker, type)[index]); /* TODO: make it simplier */
    sstor  = worker->server;

    sender->index = index;
    sender->fd    = socket(sstor->addr.ss_family, sock_type, 0);
    
    if (sender->fd < 0)
        fatal("[-] Failed to create a sender's socket");
        
    if (connect(sender->fd, (struct sockaddr *) &(sstor->addr), sstor->len) < 0) {
        log_info("[-] Worker %d: connection error --> %s:53", worker->index, worker->server->repr);
        close(sender->fd);
        return;
    }        
    /* make non-blocking */
    long flags = fcntl(sender->fd, F_GETFL, 0);
    if (fcntl(sender->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sender->fd);
        return;
    }

    if ((sender->ev_recv = event_new(worker->dnstress->evb, sender->fd, 
        EV_READ | EV_PERSIST, get_reply_callback(type), sender)) == NULL)
        fatal("[-] Failed to create sender's event");

    if (event_add(sender->ev_recv, NULL) == -1)
        fatal("[-] Failed to add sender's event");

    return;
}

static void senders_setup(struct worker_t *worker) {
    /* TCP workers*/
    for (size_t i = 0; i < worker->tcp_senders_count; i++) {
        sender_init(worker, i, TCP_TYPE);
    }

    /* UDP workers*/
    for (size_t i = 0; i < worker->udp_senders_count; i++) {
        sender_init(worker, i, UDP_TYPE);
    } 
}

void worker_init(struct dnstress_t *dnstress, size_t index) {
    struct worker_t *worker = &(dnstress->workers[index]);
    
    worker->index    = index;
    worker->dnstress = dnstress;
    worker->server   = &(dnstress->config->addrs[index]);
    worker->mode     = dnstress->config->mode;

    if (worker->mode == SHUFFLE) {
        worker->tcp_senders_count = (size_t) dnstress->max_senders / 2;
        worker->udp_senders_count = (size_t) dnstress->max_senders / 2;
    } else if (worker->mode == UDP_VALID || worker->mode == UDP_NONVALID)
        worker->udp_senders_count = (size_t) dnstress->max_senders;
    else
        worker->tcp_senders_count = (size_t) dnstress->max_senders;

    worker->tcp_senders = xmalloc_0(sizeof(struct sender_t) * worker->tcp_senders_count);
    worker->udp_senders = xmalloc_0(sizeof(struct sender_t) * worker->udp_senders_count);

    senders_setup(worker);
    log_info("[+] Workers are configured");
}

void worker_run(void *arg) {
    struct worker_t * worker = (struct worker_t *) arg;

    log_info("Worker id: %d", worker->index);

    for (size_t i = 0; i < worker->tcp_senders_count; i++) {
        /* sending DNS requests */

    }

    for (size_t i = 0; i < worker->udp_senders_count; i++) {
        /* sending DNS requests */

    }
}

void worker_clear(struct worker_t * worker) {
    for (size_t i = 0; i < worker->tcp_senders_count; i++) {
        if (worker->tcp_senders[i].ev_recv != NULL)
            event_free(worker->tcp_senders[i].ev_recv);
    }
    
    for (size_t i = 0; i < worker->udp_senders_count; i++) {
        if (worker->udp_senders[i].ev_recv != NULL)
            event_free(worker->udp_senders[i].ev_recv);
    }

    if (worker->tcp_senders != NULL) free(worker->tcp_senders);
    if (worker->udp_senders != NULL) free(worker->udp_senders);
}
