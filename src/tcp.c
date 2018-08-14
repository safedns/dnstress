#include "tcp.h"
#include "servant.h"
#include "log.h"
#include "utils.h"

void
send_tcp_query(struct servant_t *servant)
{
    /* FIXME: copy-paste with udp module */
    if (perform_query(servant, ldns_tcp_send_query) < 0)
        fatal("%s: error perfoming query", __func__);

    struct rstats_t *stats = gstats(servant);
    inc_rsts_fld(stats, &stats->n_sent_tcp);

    ldns_buffer_clear(servant->buffer);

    /* debug output */
    // fprintf(stderr, "%zu\r", stats->n_sent_tcp);
}

void
recv_tcp_reply(evutil_socket_t fd, short events, void *arg)
{
    struct servant_t *servant = (struct servant_t *) arg;

    if (recv_reply(servant, ldns_tcp_read_wire, TCP_TYPE) < 0)
        fatal("%s: error while receiving reply", __func__);
}