#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#include "worker.h"
#include "utils.h"
#include "log.h"
#include "udp.h"
#include "tcp.h"

#define TIME_INTERVAL_UDP 1000L
#define TIME_INTERVAL_TCP 1000L

#define MAX_TIME_INTERVAL 1000000000L

#define UDP_RUN_COUNT 10
#define TCP_RUN_COUNT 10

#define EPS 1e-5

struct worker_arg_t {
    struct worker_t *worker;
    request_mode_t mode;
    
    int time_interval;
    size_t run_count;
    
    bool (*mode_b)(const request_mode_t);
    ssize_t (*servant_run)(const struct servant_t *);
};

static struct worker_arg_t *
worker_arg_new(struct worker_t *worker,
               request_mode_t mode,
               int time_interval,
               size_t run_count,
               bool(*mode_b)(const request_mode_t),
               ssize_t (*servant_run)(const struct servant_t *))
{
    struct worker_arg_t *worker_arg = xmalloc_0(sizeof(struct worker_arg_t));

    if (worker_arg == NULL)  return NULL;
    if (worker == NULL)      return NULL;
    if (mode_b == NULL)      return NULL;
    if (servant_run == NULL) return NULL;

    worker_arg->worker      = worker;
    worker_arg->mode        = mode;
    worker_arg->time_interval   = time_interval;
    worker_arg->run_count   = run_count;
    worker_arg->mode_b      = mode_b;
    worker_arg->servant_run = servant_run;

    return worker_arg;
}

static int
worker_arg_free(struct worker_arg_t *worker_arg)
{
    if (worker_arg == NULL)
        return -1;

    worker_arg->worker      = NULL;
    worker_arg->mode        = 0;
    worker_arg->time_interval   = 0;
    worker_arg->run_count   = 0;
    worker_arg->mode_b      = NULL;
    worker_arg->servant_run = NULL;

    free(worker_arg);
    
    return 0;
}

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
worker_servants_setup(struct worker_t *worker)
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

/**
 * Get exactly udp or tcp serv count.
 *  If `shuffle` is specified, then it's considered as
 *  an invalid mode
 */
ssize_t
worker_serv_count(struct worker_t *worker, const request_mode_t mode)
{
    if (mode == SHUFFLE)
        return -1;

    if (tcp_mode_b(mode))
        return worker->tcp_serv_count;
    
    if (udp_mode_b(mode))
        return worker->udp_serv_count;

    return 0;
}

/**
 * Get exactly udp or tcp servants.
 * If `shuffle` is specified, then it's considered as
 * an invalid mode
*/
struct servant_t *
worker_servants(struct worker_t *worker, const request_mode_t mode)
{
    if (mode == SHUFFLE)
        return NULL;

    if (tcp_mode_b(mode))
        return worker->tcp_servants;
    
    if (udp_mode_b(mode))
        return worker->udp_servants;

    return NULL;
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

    worker_servants_setup(worker);
    log_info("servants are configured");

    worker_activate(worker);

    worker->start_time = clock();
}

static bool
rps_valid_conditions(struct dnsconfig_t *config, size_t last_round_sent,
    size_t overall_sent)
{
    if (last_round_sent + 1 <= config->rps) {
        if (config->ttl != 0) {
            if (overall_sent + 1 <= config->rps * config->ttl)
                return true;
            return false;
        } else
            return true;
    }
    return false;
}

static void
set_time_interval(struct timespec *tim, uint16_t ld_lvl,
    int time_interval)
{
    long ld_time_interval = 0;
    
    ld_time_interval = ld_lvl;

    ld_time_interval = (3000 * ((long) pow(ld_lvl - 10, 4)) + 1000) * 10;
    
    if (ld_time_interval >= MAX_TIME_INTERVAL) {
        tim->tv_sec = ld_time_interval / MAX_TIME_INTERVAL;
        tim->tv_nsec = 0;
    } else {
        tim->tv_sec = 0;
        tim->tv_nsec = ld_time_interval;
        
        // fprintf(stderr, "inter: %ld\n", ld_time_interval);
    }
}

static void *
__worker_run_inner(void *__arg)
{
    struct worker_arg_t *arg = __arg;

    struct timespec tim;
    
    size_t last_round_sent = 0;
    size_t overall_sent    = 0;

    ssize_t ret = 0;

    struct worker_t    *worker = arg->worker;
    struct dnsconfig_t *config = worker->config;

    if (!arg->mode_b(worker->mode)) {
        pthread_exit(NULL);
        return NULL;
    }

    clock_t cur_time = clock();
    double elapsed = 0;

    if (dnsconfig_ld_lvl_enabled(config)) {
        set_time_interval(&tim, config->ld_lvl, arg->time_interval);
    }

    size_t serv_count = worker_serv_count(worker, arg->mode);
    struct servant_t *servants = worker_servants(worker, arg->mode);

    while (true) {
        if (!worker_active(worker))
            break;

        if (arg->mode_b(worker->mode)) {
            for (size_t _ = 0; _ < arg->run_count; _++) {
                for (size_t i = 0; i < serv_count; i++) {
                    /* sending DNS requests */
                    if (dnsconfig_rps_enabled(config)) {
                        if (rps_valid_conditions(config, last_round_sent, overall_sent)) {
                            ret = arg->servant_run(&servants[i]);
                            if (ret > 0) {
                                last_round_sent++;
                                overall_sent++;
                            }
                        }
                        
                        elapsed = time_elapsed(cur_time);
                        
                        if (elapsed + EPS >= 1) {
                            cur_time = clock();
                            last_round_sent = 0;
                            // fprintf(stderr, "overall sent: %zu\n", overall_sent);
                        }

                        if (last_round_sent >= config->rps)
                            continue;
                    } else
                        arg->servant_run(&servants[i]);
                }
            }
        }

        if (dnsconfig_ld_lvl_enabled(config)) {
            if (nanosleep(&tim , NULL) < 0)
                log_warn("%s: failed to nanosleep", __func__);
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void
worker_run(void *arg)
{    
    struct worker_t * worker = (struct worker_t *) arg;

    pthread_t udp_worker;
    pthread_t tcp_worker;

    struct worker_arg_t *udp_arg = worker_arg_new(worker, UDP_VALID, TIME_INTERVAL_UDP,
                                                  UDP_RUN_COUNT, udp_mode_b,
                                                  udp_servant_run);
    struct worker_arg_t *tcp_arg = worker_arg_new(worker, TCP_VALID, TIME_INTERVAL_TCP,
                                                  TCP_RUN_COUNT, tcp_mode_b,
                                                  tcp_servant_run);
    if (udp_arg == NULL)
        fatal("%s: failed to create udp worker_arg", __func__);
    if (tcp_arg == NULL)
        fatal("%s: failed to create tcp worker arg", __func__);

    if (pthread_create(&udp_worker, NULL,
        __worker_run_inner, (void *) udp_arg) != 0)
        fatal("%s: failed to create udp worker thread");
    if (pthread_create(&tcp_worker, NULL,
        __worker_run_inner, (void *) tcp_arg) != 0)
        fatal("%s: failed to create tcp worker thread");

    if (pthread_join(udp_worker, NULL) != 0)
        fatal("%s: failed to join udp worker");
    if (pthread_join(tcp_worker, NULL) != 0)
        fatal("%s: failed to join udp worker");

    if (worker_arg_free(udp_arg) < 0)
        fatal("%s: failed to free udp worker arg", __func__);
    if (worker_arg_free(tcp_arg) < 0)
        fatal("%s: failed to free tcp worker arg", __func__);
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
