#include "tcp.h"
#include "servant.h"
#include "log.h"
#include "utils.h"

void
send_tcp_query(struct servant_t *servant)
{
    if (perform_query(servant, ldns_tcp_send_query) < 0)
        fatal("%s: error perfoming query", __func__);

    struct serv_stats_t *sv_stats = get_serv_stats_servant(servant);
    inc_rsts_fld(sv_stats, &sv_stats->tcp_serv.n_sent);

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