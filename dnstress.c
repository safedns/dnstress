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

int dnstress_create(dnsconfig_t *config) {
    return 0;
}

int dnstress_run(dnstress_t *dnstress) {
    return 0;
}

int dnstress_free(dnstress_t *dnstress) {
    return 0;
}

int main(int argc, char **argv) {
    dnsconfig_t config;

    size_t mode         = 0;
    size_t worker_count = 0;

    memset(&config, 0, sizeof(config));

    parse_args(argc, argv, &config);          /* parse command line arguments */
    parse_config(&config, config.configfile); /* parse config file */

#ifdef DEBUG
    fprintf(stderr, "[*] Mode: %zu    Wcount: %zu\n", mode, worker_count);
#endif

    return 0;
}