#ifndef __SERVANT_H__
#define __SERVANT_H__

#include <stdio.h>
#include <pthread.h>

#include <event2/event.h>
#include <ldns/ldns.h>

#include "worker.h"

struct worker_t;

#define TCP       "TCP"
#define UDP       "UDP"
#define UNDEFINED "UNDEFINED"

typedef enum {
    TCP_TYPE,
    UDP_TYPE,
    CLEANED
} servant_type_t;

struct servant_t {
    size_t index;  /* index in worker's `servants` list */
    int fd;
    
    servant_type_t type;
    
    struct _saddr *server;
    struct event  *ev_recv;

    struct dnsconfig_t *config;
    struct worker_t    *worker_base;

    ldns_buffer *buffer;
};

/* FIXME: change function's arguments */
void servant_init(struct worker_t *worker, size_t index, servant_type_t type);
void servant_clear(struct servant_t *servant);

void tcp_servant_run(struct servant_t *servant);
void udp_servant_run(struct servant_t *servant);

struct rstats_t * gstats(struct servant_t *servant);

#endif