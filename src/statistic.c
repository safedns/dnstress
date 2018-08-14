#include "statistic.h"
#include "utils.h"

struct rstats_t *
stats_create(void)
{
    struct rstats_t *stats = (struct rstats_t *) xmalloc_0(sizeof(struct rstats_t));
    if (pthread_mutex_init(&(stats->lock), NULL) != 0) {
        free(stats);
        return NULL;
    }

    if (pthread_cond_init(&(stats->cond), NULL) != 0) {
        free(stats);
        return NULL;
    }

    return stats;
}

void
stats_free(struct rstats_t *stats)
{
    pthread_mutex_destroy(&(stats->lock));
    pthread_cond_destroy(&(stats->cond));
    
    free(stats);
}

/* debugging function */
void
__stats_update_servant(struct rstats_t *stats, struct servant_t *servant)
{

    ldns_pkt *reply = ldns_pkt_new();
    ldns_buffer2pkt_wire(&reply, servant->buffer);
    
    if (ldns_pkt_get_rcode(reply) == LDNS_RCODE_SERVFAIL)
        log_warn("worker: %d | servant: %d | servfail!",
            servant->worker_base->index, servant->index);    

    stats_update_pkt(stats, reply);
    ldns_pkt_free(reply);
}

void
stats_update_buf(struct rstats_t *stats, const ldns_buffer *buffer)
{
    if (stats == NULL || buffer == NULL)
        fatal("%s: null argument pointer", __func__);
    
    ldns_pkt *reply = ldns_pkt_new();
    ldns_buffer2pkt_wire(&reply, buffer);
    stats_update_pkt(stats, reply);
    ldns_pkt_free(reply);
}

void
stats_update_pkt(struct rstats_t *stats, const ldns_pkt *pkt)
{
    /* TODO: too crappy code has to be changed */
    switch (ldns_pkt_get_rcode(pkt)) {
        case LDNS_RCODE_NOERROR:
            inc_rsts_fld(stats, &(stats->n_noerr));
            break;
        case LDNS_RCODE_FORMERR:
            inc_rsts_fld(stats, &(stats->n_formerr));
            break;
        case LDNS_RCODE_SERVFAIL:
            inc_rsts_fld(stats, &(stats->n_servfail));
            break;
        case LDNS_RCODE_NXDOMAIN:
            inc_rsts_fld(stats, &(stats->n_nxdomain));
            break;
        case LDNS_RCODE_NOTIMPL:
            inc_rsts_fld(stats, &(stats->n_notimpl));
            break;
        case LDNS_RCODE_REFUSED:
            inc_rsts_fld(stats, &(stats->n_refused));
            break;
        case LDNS_RCODE_YXDOMAIN:
            inc_rsts_fld(stats, &(stats->n_yxdomain));
            break;
        case LDNS_RCODE_YXRRSET:
            inc_rsts_fld(stats, &(stats->n_yxrrset));
            break;
        case LDNS_RCODE_NXRRSET:
            inc_rsts_fld(stats, &(stats->n_nxrrset));
            break;
        case LDNS_RCODE_NOTAUTH:
            inc_rsts_fld(stats, &(stats->n_notauth));
            break;
        case LDNS_RCODE_NOTZONE:
            inc_rsts_fld(stats, &(stats->n_notzone));
            break;
    }
}

void
stats_update_stats(struct rstats_t *stats1, const struct rstats_t *stats2)
{
    if (pthread_mutex_lock(&(stats1->lock)) != 0)
        fatal("%s: mutex lock error", __func__);

    stats1->n_sent_udp += stats2->n_sent_udp;
    stats1->n_recv_udp += stats2->n_recv_udp;

    stats1->n_sent_tcp += stats2->n_sent_tcp;
    stats1->n_recv_tcp += stats2->n_recv_tcp;

    stats1->n_noerr    += stats2->n_noerr;
    stats1->n_formerr  += stats2->n_formerr;
    stats1->n_servfail += stats2->n_servfail;
    stats1->n_nxdomain += stats2->n_nxdomain;
    stats1->n_notimpl  += stats2->n_notimpl;
    stats1->n_refused  += stats2->n_refused;
    stats1->n_yxdomain += stats2->n_yxdomain;
    stats1->n_yxrrset  += stats2->n_yxrrset;
    stats1->n_nxrrset  += stats2->n_nxrrset;
    stats1->n_notauth  += stats2->n_notauth;
    stats1->n_notzone  += stats2->n_notzone;

    stats1->__call_num++;

    if (pthread_mutex_unlock(&(stats1->lock)) != 0)
        fatal("%s: mutex unlock error", __func__);
}

void
print_stats(struct rstats_t *stats)
{
    /* TODO: PRINT RESULTS */

    fprintf(stderr, "\n");
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

void
inc_rsts_fld(struct rstats_t *stats, size_t *field)
{
    if (pthread_mutex_lock(&(stats->lock)) != 0)
        fatal("%s: mutex lock error", __func__);
    
    *field = *field + 1;

    if (pthread_mutex_unlock(&(stats->lock)) != 0)
        fatal("%s: mutex unlock error", __func__);
}