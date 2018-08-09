#include <unistd.h>
#include <string.h>

#include "worker.h"
#include "utils.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"

static bool
udp_mode_b(request_mode_t mode) {
    return (mode == UDP_VALID || mode == UDP_NONVALID || mode == SHUFFLE);
}

static bool
tcp_mode_b(request_mode_t mode) {
    return (mode == TCP_VALID || mode == TCP_NONVALID || mode == SHUFFLE);
}

static void 
servants_setup(struct worker_t *worker)
{
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

void
worker_init(struct dnstress_t *dnstress, size_t index)
{
    struct worker_t *worker = &(dnstress->workers[index]);
    
    worker->index  = index;

    worker->server = &(dnstress->config->addrs[index]);

    if (!is_server_available(worker->server))
        fatal("%s: server is not available: %s", __func__, worker->server->repr);

    worker->mode   = dnstress->config->mode;

    worker->dnstress = dnstress;
    worker->config   = dnstress->config;

    worker->tcp_serv_count = dnstress->max_tcp_servants;
    worker->udp_serv_count = dnstress->max_udp_servants;

    worker->tcp_servants = xmalloc_0(sizeof(struct servant_t) * worker->tcp_serv_count);
    worker->udp_servants = xmalloc_0(sizeof(struct servant_t) * worker->udp_serv_count);

    servants_setup(worker);
    log_info("workers are configured");
}

void
worker_run(void *arg)
{
    struct worker_t * worker = (struct worker_t *) arg;

    if (tcp_mode_b(worker->mode)) {
        for (size_t i = 0; i < worker->tcp_serv_count; i++) {
            /* sending DNS requests */
            send_tcp_query(&worker->tcp_servants[i]);
            // tcp_servant_run(&worker->tcp_servants[i]);
        }
    }

    if (udp_mode_b(worker->mode)) {
        for (size_t i = 0; i < worker->udp_serv_count; i++) {
            /* sending DNS requests */
            send_udp_query(&worker->udp_servants[i]);
            // udp_servant_run(&worker->udp_servants[i]);
        }
    }
}

void
worker_clear(struct worker_t * worker)
{
    for (size_t i = 0; i < worker->tcp_serv_count; i++) {
        servant_clear(&worker->tcp_servants[i]);
    }
    
    for (size_t i = 0; i < worker->udp_serv_count; i++) {
        servant_clear(&worker->udp_servants[i]);
    }

    if (worker->tcp_servants != NULL) free(worker->tcp_servants);
    if (worker->udp_servants != NULL) free(worker->udp_servants);

    worker->tcp_servants = NULL;
    worker->udp_servants = NULL;
    worker->server       = NULL;
    worker->dnstress     = NULL;
    worker->config       = NULL;
}
