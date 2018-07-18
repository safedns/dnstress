#include <fcntl.h>
#include <unistd.h>

#include "worker.h"
#include "utils.h"

static int get_sock_type(request_mode_t mode) {
    if (mode == UDP_VALID || mode == UDP_NONVALID)
        return SOCK_DGRAM;
    else return SOCK_STREAM;
}


static void sender_reply(evutil_socket_t fd, short events, void *arg) {
    struct sender_t *sender = (struct sender_t *) arg;
}

static void senders_setup(struct worker_t *worker) {
    int sock_type = get_sock_type(worker->mode);
    struct sockaddr_storage *sstor = NULL;
    struct sender_t *sender = NULL;

    for (size_t i = 0; i < worker->senders_count; i++) {
        /* FIXME: some memory corruption */
    
        sender = &(worker->senders[i]);
        sstor  = worker->server;

        sender->index = i;
        sender->fd    = socket(sstor->ss_family, sock_type, 0);
        
        if (sender->fd < 0) fatal("[-] Failed to create a sender's socket");
        if (connect(sender->fd, (struct sockaddr *)&sstor, sizeof(sstor)) < 0) {
            close(sender->fd);
            continue;
        }        
        /* make non-blocking */
        long flags = fcntl(sender->fd, F_GETFL, 0);
	    if (fcntl(sender->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(sender->fd);
            continue;
        }

        if ((sender->ev_recv = event_new(worker->dnstress->evb, sender->fd, 
            EV_READ | EV_PERSIST, sender_reply, sender)) == NULL)
            fatal("[-] Failed to create sender's event");
        if (event_add(sender->ev_recv, NULL) == -1)
            fatal("[-] Failed to add sender's event");
    }
}

void worker_init(struct dnstress_t *dnstress, size_t index) {
    struct worker_t *worker = &(dnstress->workers[index]);
    
    worker->dnstress      = dnstress;
    worker->server        = &(dnstress->config->addrs[index]);
    worker->senders_count = dnstress->max_senders;
    worker->mode          = dnstress->config->mode;

    worker->senders = xmalloc_0(sizeof(struct sender_t) * dnstress->max_senders);

    senders_setup(worker);
}

void worker_run(void *arg) {
    struct worker_t * worker = (struct worker_t *) arg;
    
}

void worker_clear(struct worker_t * worker) {
    for (size_t i = 0; i < worker->senders_count; i++) {
        if (worker->senders[i].ev_recv != NULL)
            event_free(worker->senders[i].ev_recv);
    }
    if (worker->senders != NULL) free(worker->senders);
}