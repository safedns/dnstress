/**
 * dnstress is a tool for generating DNS traffic in
 * different modes for DNS servers. In other words,
 * this is a stress-test perfomance tool. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argparser.h"
#include "dnstress.h"
#include "utils.h"

#define MAX_SENDERS 200

struct dnstress_t * dnstress_create(dnsconfig_t *config) {
    struct dnstress_t *dnstress = xmalloc_0(sizeof(struct dnstress_t));
    
    dnstress->config = config;
    dnstress->workers_count = config->addrs_count;

    dnstress->max_senders = MAX_SENDERS;

    dnstress->workers = xmalloc_0(sizeof(struct worker_t) * dnstress->workers_count);

    for (size_t i = 0; i < dnstress->workers_count; i++) {
        worker_init(dnstress, i);
        // worker_init(&dnstress->workers[i], &config->addrs[i], MAX_SENDERS, config->mode);
    }

    if ((dnstress->evb = event_base_new()) == NULL)
		fatal("[-] Failed to create event base");
    if ((dnstress->pool = thread_pool_init(MAX_THREADS, QUEUE_SIZE)) == NULL)
        fatal("[-] Failed to create thread pool");

    return dnstress;
}

int dnstress_run(struct dnstress_t *dnstress) {
    /* put all works in a queue */
    for (size_t i = 0; i < dnstress->workers_count; i++) {
        thread_pool_add(dnstress->pool, &worker_run, &(dnstress->workers[i]));
    }
    return 0;
}

int dnstress_free(struct dnstress_t *dnstress) {
    if (dnstress == NULL) return 0;

    dnsconfig_free(dnstress->config);
    
    for (size_t i = 0; i < dnstress->workers_count; i++)
        worker_clear(&(dnstress->workers[i]));
    
    event_base_free(dnstress->evb);
    thread_pool_kill(dnstress->pool, complete_shutdown);

    free(dnstress->workers);
    free(dnstress);
    
    return 0;
}

int main(int argc, char **argv) {
    dnsconfig_t *config   = NULL;
    struct dnstress_t  *dnstress = NULL;

    config = dnsconfig_create();

    parse_args(argc, argv, config);
    if (parse_config(config, config->configfile) < 0) fatal("[-] Error while parsing config");

    dnstress = dnstress_create(config);
    dnstress_run(dnstress);

    dnstress_free(dnstress);

    return 0;
}