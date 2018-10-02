#include "udp.h"
#include "network.h"
#include "statistic.h"
#include "utils.h"

static ssize_t
__send_query(ldns_buffer *qbin, int sockfd,
    const struct sockaddr_storage *to, socklen_t tolen)
{
    return -1;   
}

ssize_t
send_udp_query(const struct servant_t *servant)
{
    ssize_t sent = 0;
    if ((sent = perform_query(servant, ldns_udp_send_query)) < 0) {
        // log_warn("%s: error perfoming query", __func__);
        return -1;
    }
    if (sent > 0) {
        struct serv_stats_t *sv_stats = get_serv_stats_servant(servant);
        inc_rsts_fld(sv_stats, &sv_stats->udp_serv.n_sent);
    }
    return sent;
}

void
recv_udp_reply(evutil_socket_t fd, short events, void *arg)
{
    struct servant_t *servant = (struct servant_t *) arg;

    if (recv_reply(servant, ldns_udp_read_wire, UDP_TYPE) < 0)
        fatal("%s: error while receiving reply", __func__);
}