#include <unistd.h>
#include <string.h>

#include "worker.h"
#include "utils.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"

static void servants_setup(struct worker_t *worker) {
    /* TCP workers*/
    for (size_t i = 0; i < worker->tcp_serv_count; i++) {
        servant_init(worker, i, TCP_TYPE);
    }
    log_info("worker: %d | servants: [0-%d]/TCP",
            worker->index, worker->tcp_serv_count);
    
    /* UDP workers*/
    for (size_t i = 0; i < worker->udp_serv_count; i++) {
        servant_init(worker, i, UDP_TYPE);
    }
    log_info("worker: %d | servants: [0-%d]/UDP",
            worker->index, worker->udp_serv_count);
}

void worker_init(struct dnstress_t *dnstress, size_t index) {
    struct worker_t *worker = &(dnstress->workers[index]);
    
    worker->index    = index;
    worker->server   = &(dnstress->config->addrs[index]);
    worker->mode     = dnstress->config->mode;

    worker->dnstress = dnstress;
    worker->config   = dnstress->config;

    if (worker->mode == SHUFFLE) {
        worker->tcp_serv_count = (size_t) dnstress->max_servants / 2;
        worker->udp_serv_count = (size_t) dnstress->max_servants / 2;
    } else if (worker->mode == UDP_VALID || worker->mode == UDP_NONVALID)
        worker->udp_serv_count = (size_t) dnstress->max_servants;
    else
        worker->tcp_serv_count = (size_t) dnstress->max_servants;

    worker->tcp_servants = xmalloc_0(sizeof(struct servant_t) * worker->tcp_serv_count);
    worker->udp_servants = xmalloc_0(sizeof(struct servant_t) * worker->udp_serv_count);

    servants_setup(worker);
    log_info("workers are configured");
}

void worker_run(void *arg) {
    struct worker_t * worker = (struct worker_t *) arg;

    for (size_t i = 0; i < worker->tcp_serv_count; i++) {
        /* sending DNS requests */
        send_tcp_query(&worker->tcp_servants[i]);
        // tcp_servant_run(&worker->tcp_servants[i]);
    }

    for (size_t i = 0; i < worker->udp_serv_count; i++) {
        /* sending DNS requests */
        send_udp_query(&worker->udp_servants[i]);
        // udp_servant_run(&worker->udp_servants[i]);
    }
}

void worker_clear(struct worker_t * worker) {
    for (size_t i = 0; i < worker->tcp_serv_count; i++) {
        if (worker->tcp_servants[i].ev_recv != NULL)
            event_free(worker->tcp_servants[i].ev_recv);
    }
    
    for (size_t i = 0; i < worker->udp_serv_count; i++) {
        if (worker->udp_servants[i].ev_recv != NULL)
            event_free(worker->udp_servants[i].ev_recv);
    }

    if (worker->tcp_servants != NULL) free(worker->tcp_servants);
    if (worker->udp_servants != NULL) free(worker->udp_servants);

    worker->tcp_servants = NULL;
    worker->udp_servants = NULL;
    worker->server       = NULL;
    worker->dnstress     = NULL;
}
