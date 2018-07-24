#include "statistic.h"
#include "utils.h"

struct rstats_t * stats_create(void) {
    struct rstats_t *stats = (struct rstats_t *) xmalloc_0(sizeof(struct rstats_t));
    return stats;
}

void stats_free(struct rstats_t *stats) {
    free(stats);
}

/* debugging function */
void __stats_update_servant(struct rstats_t *stats, struct servant_t *servant) {

    ldns_pkt *reply = ldns_pkt_new();
    ldns_buffer2pkt_wire(&reply, servant->buffer);
    
    if (ldns_pkt_get_rcode(reply) == LDNS_RCODE_SERVFAIL)
        log_warn("worker: %d | servant: %d | servfail!",
            servant->worker_base->index, servant->index);    

    stats_update_pkt(stats, reply);
    ldns_pkt_free(reply);
}

void stats_update_buf(struct rstats_t *stats, const ldns_buffer *buffer) {
    ldns_pkt *reply = ldns_pkt_new();
    ldns_buffer2pkt_wire(&reply, buffer);
    stats_update_pkt(stats, reply);
    ldns_pkt_free(reply);
}

/* FIXME: lock */
void stats_update_pkt(struct rstats_t *stats, const ldns_pkt *pkt) {
    /* TODO: too crappy code have to be changed */
    switch (ldns_pkt_get_rcode(pkt)) {
        case LDNS_RCODE_NOERROR:
            stats->n_noerr++;
            break;
        case LDNS_RCODE_FORMERR:
            stats->n_formerr++;
            break;
        case LDNS_RCODE_SERVFAIL:
            stats->n_servfail++;
            break;
        case LDNS_RCODE_NXDOMAIN:
            stats->n_nxdomain++;
            break;
        case LDNS_RCODE_NOTIMPL:
            stats->n_notimpl++;
            break;
        case LDNS_RCODE_REFUSED:
            stats->n_refused++;
            break;
        case LDNS_RCODE_YXDOMAIN:
            stats->n_yxdomain++;
            break;
        case LDNS_RCODE_YXRRSET:
            stats->n_yxrrset++;
            break;
        case LDNS_RCODE_NXRRSET:
            stats->n_nxrrset++;
            break;
        case LDNS_RCODE_NOTAUTH:
            stats->n_notauth++;
            break;
        case LDNS_RCODE_NOTZONE:
            stats->n_notzone++;
            break;
    }
}

void print_stats(struct rstats_t *stats) {
    /* TODO: PRINT RESULTS */

    fprintf(stderr, "   TEST STATISTIC\n");
    fprintf(stderr, "     ==== UDP ====\n");
    fprintf(stderr, "     [*] sent UDP packets: %zu\n", stats->n_sent_udp);
    fprintf(stderr, "     [*] recv UDP packets: %zu\n", stats->n_recv_udp);
    fprintf(stderr, "                  \n");
    fprintf(stderr, "     ==== TCP ====\n");
    fprintf(stderr, "     [*] sent TCP packets: %zu\n", stats->n_sent_tcp);
    fprintf(stderr, "     [*] recv TCP packets: %zu\n", stats->n_recv_tcp);
    fprintf(stderr, "                  \n");
    fprintf(stderr, "     ==== ERRORS ====\n");
    fprintf(stderr, "     [+] NO ERRORS:       %zu\n", stats->n_noerr);
    fprintf(stderr, "     [-] FORMAT ERROR:    %zu\n", stats->n_formerr);
    fprintf(stderr, "     [-] SERVER FAILURE:  %zu\n", stats->n_servfail);
    fprintf(stderr, "     [-] NAME ERROR:      %zu\n", stats->n_nxdomain);
    fprintf(stderr, "     [-] NOT IMPLEMENTED: %zu\n", stats->n_notimpl);
    fprintf(stderr, "     [-] REFUSED:         %zu\n", stats->n_refused);
    fprintf(stderr, "\n");
    fprintf(stderr, "Bye!\n");
}