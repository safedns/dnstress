#ifndef __SERVANT_H__
#define __SERVANT_H__

#include <stdio.h>

#include <event2/event.h>
#include <ldns/ldns.h>

#include "worker.h"

struct worker_t;

typedef enum {
    TCP_TYPE,
    UDP_TYPE
} servant_type_t;

struct servant_t {
    size_t index;  /* index in worker's `servants` list */
    int fd;
    
    servant_type_t type;
    
    struct sockaddr_storage *server;
    struct event *ev_recv;

    struct dnsconfig_t *config;
};

/* FIXME: change function's arguments */
void servant_init(struct worker_t *worker, size_t index, servant_type_t type);

void tcp_servant_run(struct servant_t *servant);
void udp_servant_run(struct servant_t *servant);

#endif