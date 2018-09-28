#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "worker.h"
#include "utils.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"

#define TIME_STEP_UDP 4000000L
#define TIME_STEP_TCP 5000000L

#define UDP_RUN_COUNT 8
#define TCP_RUN_COUNT 4

static bool
udp_mode_b(const request_mode_t mode)
{
    return (mode == UDP_VALID || mode == UDP_NONVALID || mode == SHUFFLE);
}

static bool
tcp_mode_b(const request_mode_t mode)
{
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

bool
worker_active(struct worker_t *worker)
{
    bool active = false;
    
    if (pthread_mutex_lock(&worker->lock) < 0)
        fatal("%s: failed to lock", __func__);
    active = worker->active;
    if (pthread_mutex_unlock(&worker->lock) < 0)
        fatal("%s: failed to unlock", __func__);
    return active;
}

void
worker_activate(struct worker_t *worker)
{
    if (pthread_mutex_lock(&worker->lock) < 0)
        fatal("%s: failed to lock", __func__);
    worker->active = true;
    if (pthread_mutex_unlock(&worker->lock) < 0)
        fatal("%s: failed to unlock", __func__);
}

void
worker_deactivate(struct worker_t *worker)
{
    if (pthread_mutex_lock(&worker->lock) < 0)
        fatal("%s: failed to lock", __func__);
    worker->active = false;
    if (pthread_mutex_unlock(&worker->lock) < 0)
        fatal("%s: failed to unlock", __func__);
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

    worker_activate(worker);
}

static void *
worker_run_udp(void *arg)
{
    struct worker_t *worker = arg;
    struct timespec tim = { 0, TIME_STEP_UDP };

    while (true) {
        if (!worker_active(worker))
            break;

        if (udp_mode_b(worker->mode))
            for (size_t _ = 0; _ < UDP_RUN_COUNT; _++)
                for (size_t i = 0; i < worker->udp_serv_count; i++)
                    /* sending DNS requests */
                    udp_servant_run(&worker->udp_servants[i]);
        
        if (nanosleep(&tim , NULL) < 0)
            log_warn("%s: failed to nanosleep", __func__);
    }
    pthread_exit(NULL);
}

static void *
worker_run_tcp(void *arg)
{
    struct worker_t *worker = arg;
    struct timespec tim = { 0, TIME_STEP_TCP };

    while (true) {
        if (!worker_active(worker))
            break;

        if (tcp_mode_b(worker->mode))
            for (size_t _ = 0; _ < TCP_RUN_COUNT; _++)
                for (size_t i = 0; i < worker->tcp_serv_count; i++)
                    /* sending DNS requests */
                    tcp_servant_run(&worker->tcp_servants[i]);
        
        if (nanosleep(&tim , NULL) < 0)
            log_warn("%s: failed to nanosleep", __func__);
    }
    pthread_exit(NULL);
}

void
worker_run(void *arg)
{    
    struct worker_t * worker = (struct worker_t *) arg;

    pthread_t udp_worker;
    pthread_t tcp_worker;

    if (pthread_create(&udp_worker, NULL, worker_run_udp, (void *) worker) != 0)
        fatal("%s: failed to create udp worker thread");
    if (pthread_create(&tcp_worker, NULL, worker_run_tcp, (void *) worker) != 0)
        fatal("%s: failed to create tcp worker thread");

    if (pthread_join(udp_worker, NULL) != 0)
        fatal("%s: failed to join udp worker");
    if (pthread_join(tcp_worker, NULL) != 0)
        fatal("%s: failed to join udp worker");
}

void
worker_clear(struct worker_t *worker)
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

    worker_deactivate(worker);

    if (pthread_mutex_destroy(&worker->lock) != 0)
        fatal("%s: failed to destroy mutex", __func__);

    worker->tcp_servants = NULL;
    worker->udp_servants = NULL;
    worker->server       = NULL;
    worker->dnstress     = NULL;
    worker->config       = NULL;
}
