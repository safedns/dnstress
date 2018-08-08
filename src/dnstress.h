#ifndef __DNSTRESS_H__
#define __DNSTRESS_H__

#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>

#include "dnsconfig.h"
#include "worker.h"
#include "threadpool.h"
#include "statistic.h"

struct dnsconfig_t;

struct dnstress_t {
    struct dnsconfig_t *config;
    
    struct worker_t *workers;
    size_t workers_count; /* same as addrs_count */

    thread_pool_t *pool; /* each worker is a threadpool worker */

    struct event_base *evb;   /* event base */
    struct event *ev_sigint;  /* sigint event */
    struct event *ev_sigterm; /* sigterm event */
    struct event *ev_pipe;

    size_t max_udp_servants;
    size_t max_tcp_servants;

    struct rstats_t *stats;
};

struct process_pipes {
    int proc_fd[2];
};

enum {
	MASTER_PROC_FD,
	WORKER_PROC_FD
};

<<<<<<< HEAD
struct dnstress_t * dnstress_create(struct dnsconfig_t *config, int fd);
=======
struct dnstress_t * dnstress_create(dnsconfig_t *config, int fd);
>>>>>>> 5a25b22be79272a3825a6eca9fbbe2de9a4525f3
int dnstress_run(struct dnstress_t *dnstress);
int dnstress_free(struct dnstress_t *dnstress);

#endif /* __DNSTRESS_H__ */