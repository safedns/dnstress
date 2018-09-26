#include <unistd.h>
#include <string.h>
#include <time.h>

#include "worker.h"
#include "utils.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"

#define TIME_STEP 500000000L

#define UDP_RUN_COUNT 1
#define TCP_RUN_COUNT 10

static bool
udp_mode_b(const request_mode_t mode) {
    return (mode == UDP_VALID || mode == UDP_NONVALID || mode == SHUFFLE);
}

static bool
tcp_mode_b(const request_mode_t mode) {
    return (mode == TCP_VALID || mode == TCP_NONVALID || mode == SHUFFLE);
}

static void 
servants_setup(struct worker_t *worker)
{
    if (worker == NULL)
        fatal("%s: null pointer to a worker");
    
    /* TCP workers*/
    for (size_t i = 0; i < worker->tcp_serv_count; i++) {
        if (servant_init(worker, i, TCP_TYPE) < 0)
            fatal("%s: error while initializing a TCP servant", __func__);
    }
    log_info("worker: %d | servants: [0-%d]/TCP",
            worker->index, worker->tcp_serv_count);
    
    /* UDP workers*/
    for (size_t i = 0; i < worker->udp_serv_count; i++) {
        if (servant_init(worker, i, UDP_TYPE) < 0)
            fatal("%s: error while initializing a UDP servant", __func__);
    }
    log_info("worker: %d | servants: [0-%d]/UDP",
            worker->index, worker->udp_serv_count);
}

void
worker_init(struct dnstress_t *dnstress, const size_t index)
{
    if (dnstress == NULL)
        fatal("%s: null pointer to dnstress", __func__);
    if (index < 0 || index >= MAX_WCOUNT)
        fatal("%s: invalid index of a worker", __func__);
    
    struct worker_t *worker = &(dnstress->workers[index]);
    
    worker->index  = index;

    worker->server = &(dnstress->config->addrs[index]);

    if (!is_server_available(worker->server))
        fatal("%s: server is not available: %s", __func__, worker->server->repr);

    worker->mode = dnstress->config->mode;

    worker->dnstress = dnstress;
    worker->config   = dnstress->config;

    worker->tcp_serv_count = dnstress->max_tcp_servants;
    worker->udp_serv_count = dnstress->max_udp_servants;

    worker->tcp_servants = xmalloc_0(sizeof(struct servant_t) * worker->tcp_serv_count);
    worker->udp_servants = xmalloc_0(sizeof(struct servant_t) * worker->udp_serv_count);

    if (worker->tcp_servants == NULL)
        fatal("%s: null pointer to tcp servants", __func__);
    if (worker->udp_servants == NULL)
        fatal("%s: null pointer to udp servants", __func__);

    if (pthread_mutex_init(&worker->lock, NULL) != 0)
        fatal("%s: failed to create mutex", __func__);

    servants_setup(worker);
    log_info("servants are configured");

    worker->active = true;
}

void
worker_run(void *arg)
{
    struct worker_t * worker = (struct worker_t *) arg;
    struct timespec tim = { 0, TIME_STEP };

    struct dnsconfig_t *config = worker->config;

    clock_t start_time, cur_time;
    size_t time_elapsed = 0;
    size_t ttl = config->ttl;

    start_time = clock();

    /* infinitive loop of sending dns requests */
    while (true) {
        cur_time = clock();
        time_elapsed = (cur_time - start_time) / CLOCKS_PER_SEC;

        if (time_elapsed >= 1) {
            start_time = clock();
            /* TODO: count average number or requests per second */
        }

        pthread_mutex_lock(&worker->lock);
        if (!worker->active) {
            pthread_mutex_unlock(&worker->lock);
            break;
        }

        if (tcp_mode_b(worker->mode)) {
            for (size_t _ = 0; _ < TCP_RUN_COUNT; _++) {
                for (size_t i = 0; i < worker->tcp_serv_count; i++) {
                    /* sending DNS requests */
                    tcp_servant_run(&worker->tcp_servants[i]);
                    // send_tcp_query(&worker->tcp_servants[i]);
                }
            }
        }

        if (udp_mode_b(worker->mode)) {
            for (size_t _ = 0; _ < UDP_RUN_COUNT; _++) {
                for (size_t i = 0; i < worker->udp_serv_count; i++) {
                    /* sending DNS requests */
                    udp_servant_run(&worker->udp_servants[i]);
                    // udp_servant_run(&worker->udp_servants[i]);
                }
            }
        }
        pthread_mutex_unlock(&worker->lock);
        
        if (nanosleep(&tim , NULL) < 0)
            log_warn("%s: failed to nanosleep", __func__);
    }
    
    /* FIXME: wait for responses */
}

void
worker_clear(struct worker_t * worker)
{
    for (size_t i = 0; i < worker->tcp_serv_count; i++) {
        if (servant_clear(&worker->tcp_servants[i]) < 0)
            fatal("%s: failed to clear servant", __func__);
    }
    
    for (size_t i = 0; i < worker->udp_serv_count; i++) {
        if (servant_clear(&worker->udp_servants[i]) < 0)
            fatal("%s: failed to clear servant", __func__);
    }

    if (worker->tcp_servants != NULL) free(worker->tcp_servants);
    if (worker->udp_servants != NULL) free(worker->udp_servants);

    if (pthread_mutex_destroy(&worker->lock) != 0)
        fatal("%s: failed to destroy mutex", __func__);

    worker->tcp_servants = NULL;
    worker->udp_servants = NULL;
    worker->server       = NULL;
    worker->dnstress     = NULL;
    worker->config       = NULL;

    worker->active = false;
}
