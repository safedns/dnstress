#ifndef __WORKER_H__
#define __WORKER_H__

#include <stdio.h>
#include <stdlib.h>

#include "network.h"
#include "servant.h"
#include "dnstress.h"

struct worker_t {
    size_t index; /* worker's index in a dnstress' list of workers */
    request_mode_t mode; /* type of requesting */

    struct _saddr *server; /* address of DNS server */
    
    struct servant_t *tcp_servants; /* tcp subworkers in a worker that send packets */
    size_t tcp_serv_count;

    struct servant_t *udp_servants; /* udp subworkers in a worker that send packets */
    size_t udp_serv_count;

    struct dnstress_t  *dnstress;
    struct dnsconfig_t *config;
};

void worker_init(struct dnstress_t *dnstress, size_t index);
void worker_run(void *arg);
void worker_clear(struct worker_t * worker);

#endif /* __WORKER_H__ */