#include "udp.h"
#include "network.h"
#include "servant.h"
#include "statistic.h"
#include "utils.h"

void send_udp_query(struct servant_t *servant) {
    ldns_buffer_clear(servant->buffer);
    query_create(servant->config, servant->buffer);

    if (servant->buffer == NULL) {
        log_warn("null servant's buffer pointer");  
        return;
    }

    ssize_t sent = ldns_udp_send_query(servant->buffer, servant->fd, 
            &servant->server->addr, servant->server->len);
    if (sent == 0) {
        /** 
         * FIXME
         * We sent 0 bytes. So, it's either a fd is broken or
         * a server is unvailable, hence let's clear the whole servant.
         * 
         * But of course it would be better to try to create a new
         * connection, and if it fails, then clear the servant
         */
        servant_clear(servant);
        log_warn("worker: %d | servant: %d/%s | sent 0 bytes", 
            servant->worker_base->index, type2str(servant->type), servant->index);
        return;
    }
}

void recv_udp_reply(evutil_socket_t fd, short events, void *arg) {
    struct servant_t *servant = (struct servant_t *) arg;
    struct rstats_t  *stats   = servant->worker_base->dnstress->stats;

    size_t   answer_size = 0;
    uint8_t *answer      = NULL;

    answer = ldns_udp_read_wire(servant->fd, &answer_size, NULL, NULL);
    if (answer == NULL)
        log_warn("worker: %d | servant: %d/%s | received empty answer",
            servant->worker_base->index, type2str(servant->type), servant->index);
    
    ldns_buffer_new_frm_data(servant->buffer, answer, answer_size);
    stats_update_buf(stats, servant->buffer);
}