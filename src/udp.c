#include "udp.h"
#include "network.h"
#include "statistic.h"
#include "utils.h"

void send_udp_query(struct servant_t *servant) {
    query_create(servant);

    if (servant->server == NULL) {
        fatal("null pointer at servant server");
    }

    ssize_t sent = ldns_udp_send_query(servant->buffer, servant->fd, 
            &servant->server->addr, servant->server->len);
    
    if (sent <= 0) {
        /** 
         * FIXME
         * We sent 0 bytes. So, it's either a fd is broken or
         * a server is unvailable
         */
        log_warn("worker: %d | servant: %d/%s | sent %ld bytes", 
            servant->worker_base->index, servant->index, 
            type2str(servant->type), sent);
    }
    
    struct rstats_t *stats = gstats(servant);
    inc_rsts_fld(stats, &(stats->n_sent_udp));
}

void recv_udp_reply(evutil_socket_t fd, short events, void *arg) {
    struct servant_t *servant = (struct servant_t *) arg;

    size_t   answer_size = 0;
    uint8_t *answer      = NULL;

    answer = ldns_udp_read_wire(servant->fd, &answer_size, NULL, NULL);
    
    if (answer == NULL) {
        log_warn("worker: %d | servant: %d/%s | received empty answer",
            servant->worker_base->index, servant->index, type2str(servant->type));
        return;
    }
    reply_process(servant, answer, answer_size);
}