#ifndef __WORKER_H__
#define __WORKER_H__

#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>
#include <ldns/ldns.h>

#include "network.h"
#include "dnstress.h"

typedef enum {
    TCP_TYPE,
    UDP_TYPE
} sender_type_t;

struct worker_t {
    size_t index; /* worker's index in a dnstress' list of workers */
    request_mode_t mode; /* type of requesting */

    struct _saddr *server; /* address of DNS server */
    
    struct sender_t *tcp_senders; /* tcp subworkers in a worker that send packets */
    size_t tcp_senders_count;

    struct sender_t *udp_senders; /* udp subworkers in a worker that send packets */
    size_t udp_senders_count;

    struct dnstress_t *dnstress;
};

struct sender_t {
    size_t index;  /* index in worker's `senders` list */
    int fd;
    
    sender_type_t type;
    
    struct sockaddr_storage *server;
    struct event *ev_recv;
};

void worker_init(struct dnstress_t *dnstress, size_t index);
void worker_run(void *arg);
void worker_clear(struct worker_t * worker);

#endif /* __WORKER_H__ */