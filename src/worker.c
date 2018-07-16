#include "worker.h"

int worker_init(worker_t *worker, struct sockaddr_storage *server) {
    worker->server = server;
    
    return 0;
}

int worker_run(worker_t *worker) {
    return 0;
}

void worker_clear(worker_t * worker) {

}