#ifndef __DNSTRESS_H__
#define __DNSTRESS_H__

#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>

#include "dnsconfig.h"
#include "worker.h"

typedef struct dnstress_t dnstress_t;

struct dnstress_t {
    dnsconfig_t *config;
    
    worker_t    *workers;
    size_t       workers_count;

    struct event *evb;
};

dnstress_t * dnstress_create(dnsconfig_t *config);
int dnstress_run   (dnstress_t *dnstress);
int dnstress_free  (dnstress_t *dnstress);


#endif /* __DNSTRESS_H__ */