#include "udp.h"
#include "network.h"
#include "statistic.h"
#include "utils.h"

static ssize_t
__send_query(ldns_buffer *qbin, int sockfd,
    const struct sockaddr_storage *to, socklen_t tolen)
{
    ssize_t bytes = 0;
    size_t buffer_position = ldns_buffer_position(qbin);

    if (buffer_position <= 0)
        return -1;

    bytes = sendto(sockfd, (void *) ldns_buffer_begin(qbin),
        buffer_position, 0, (struct sockaddr *) to, tolen);

    if (bytes <= 0)
        return -1;
    
    if (bytes != buffer_position)
        return -1;

    return bytes;
}

ssize_t
send_udp_query(const struct servant_t *servant)
{
    ssize_t sent = 0;
    if ((sent = perform_query(servant, __send_query)) < 0) { // ldns_udp_send_query
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