#include "tcp.h"
#include "servant.h"
#include "log.h"
#include "utils.h"

void send_tcp_query(struct servant_t *servant) {
    if (!servant->active) return;
    
    query_create(servant);

    ssize_t sent = ldns_tcp_send_query(servant->buffer, servant->fd,
        &servant->server->addr, servant->server->len);
    
    if (sent <= 0) {
        fatal("worker: %d | servant: %d/%s | sent %ld bytes", 
            servant->worker_base->index, servant->index, 
            type2str(servant->type), sent);
    }

    struct rstats_t *stats = gstats(servant);
    inc_rsts_fld(stats, &(stats->n_sent_tcp));
}

void recv_tcp_reply(evutil_socket_t fd, short events, void *arg) {
    struct servant_t *servant = (struct servant_t *) arg;

    if (!servant->active) return;

    size_t   answer_size = 0;
    uint8_t *answer      = NULL;

    answer = ldns_tcp_read_wire(servant->fd, &answer_size);
    
    if (answer == NULL) {
        log_warn("worker: %d | servant: %d/%s | received empty answer",
            servant->worker_base->index, servant->index, type2str(servant->type));
        servant->active = false;
        return;
    }
    reply_process(servant, answer, answer_size);
}