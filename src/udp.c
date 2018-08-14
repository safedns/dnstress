#include "udp.h"
#include "network.h"
#include "statistic.h"
#include "utils.h"

void
send_udp_query(struct servant_t *servant)
{
    /* FIXME: copy-paste with tcp module */
    if (perform_query(servant, ldns_udp_send_query) < 0)
        fatal("%s: error perfoming query", __func__);

    struct rstats_t *stats = gstats(servant);
    inc_rsts_fld(stats, &stats->n_sent_udp);

    /* debug output */
    // fprintf(stderr, "%zu\r", stats->n_sent_udp);
}

void
recv_udp_reply(evutil_socket_t fd, short events, void *arg)
{
    struct servant_t *servant = (struct servant_t *) arg;

    if (recv_reply(servant, ldns_udp_read_wire, UDP_TYPE) < 0)
        fatal("%s: error while receiving reply", __func__);
}