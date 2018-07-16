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

dnstress_t * dnstress_create(dnsconfig_t *config) {
    dnstress_t *dnstress = xmalloc_0(sizeof(dnstress_t));
    
    dnstress->config = config;
    dnstress->workers_count = config->addrs_count;

    dnstress->workers = xmalloc_0(sizeof(worker_t) * dnstress->workers_count);

    for (size_t i = 0; i < dnstress->workers_count; i++) {
        if (worker_init(&dnstress->workers[i], &config->addrs[i]) != 0) {
            fatal("[-] Error at initialization a worker");
        }
    }

    return dnstress;
}

int dnstress_run(dnstress_t *dnstress) {
    for (size_t i = 0; i < dnstress->workers_count; i++) {
        if (worker_run(&dnstress->workers[i]) != 0) {
            fatal("[-] Error at running a worker");
        }
    }
    return 0;
}

int dnstress_free(dnstress_t *dnstress) {
    if (dnstress == NULL) return 0;

    dnsconfig_free(dnstress->config);
    
    for (size_t i = 0; i < dnstress->workers_count; i++) {
        worker_clear(&dnstress->workers[i]);
    }
    
    free(dnstress->workers);
    free(dnstress);
    
    return 0;
}

int main(int argc, char **argv) {
    dnsconfig_t *config   = NULL;
    dnstress_t  *dnstress = NULL;

    config = dnsconfig_create();

    parse_args(argc, argv, config);
    if (parse_config(config, config->configfile) < 0) fatal("[-] Error while parsing config");

    dnstress = dnstress_create(config);
    dnstress_run(dnstress);

#ifdef DEBUG
    fprintf(stderr, "[*] Mode: %d    Wcount: %zu\n", dnstress->config->mode, dnstress->config->wcount);
#endif

    dnstress_free(dnstress);

    return 0;
}