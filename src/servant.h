#ifndef __SERVANT_H__
#define __SERVANT_H__

#include <stdio.h>
#include <pthread.h>

#include <event2/event.h>
#include <ldns/ldns.h>

typedef enum {
    TCP_TYPE,
    UDP_TYPE,
    CLEANED
} servant_type_t;

#include "worker.h"

struct worker_t;

#define TCP       "TCP"
#define UDP       "UDP"
#define UNDEFINED "UNDEFINED"

typedef enum {
    CREATE_ERROR    = -1,
    NULL_WORKER     = -2,
    INCORRECT_INDEX = -3,
    CLEANED_ERROR   = -4
} servant_error_t;

struct servant_t {
    size_t index;  /* index in worker's `servants` list */
    int    fd;

    bool active;
    
    servant_type_t type;
    
    struct _saddr *server;
    
    struct event  *ev_recv;
    struct event  *ev_timeout;

    struct dnsconfig_t *config;
    struct worker_t    *worker_base;

    ldns_buffer *buffer;
};

/* FIXME: change function's arguments */
int servant_init(struct worker_t *worker,
    const size_t index, const servant_type_t type);
int servant_clear(struct servant_t *servant);

void tcp_servant_run(const struct servant_t *servant);
void udp_servant_run(const struct servant_t *servant);

struct rstats_t *gstats(const struct servant_t *servant);

#endif