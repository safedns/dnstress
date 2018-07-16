#ifndef __WORKER_H__
#define __WORKER_H__

#include <stdio.h>
#include <stdlib.h>

#include <event2/event.h>

#include "network.h"
#include "dnsconfig.h"

typedef struct worker_t worker_t;

struct worker_t {
    struct sockaddr_storage *server;
    request_mode_t mode;

    int fd;
    struct event *ev_recv;
};

int  worker_init(worker_t *worker, struct sockaddr_storage *server);
int  worker_run(worker_t *worker);
void worker_clear(worker_t * worker);

#endif /* __WORKER_H__ */