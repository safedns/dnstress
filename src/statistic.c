#include "statistic.h"
#include "utils.h"

struct rstats_t *
stats_create(void)
{
    struct rstats_t *stats = (struct rstats_t *) xmalloc_0(sizeof(struct rstats_t));

    if (stats == NULL)
        fatal("%s: failed to malloc stats");

    if (pthread_mutex_init(&(stats->lock), NULL) != 0)
        goto err_ret;

    if (pthread_cond_init(&(stats->cond), NULL) != 0)
        goto err_ret;

    return stats;

err_ret:
    free(stats);
    return NULL;
}

void
stats_free(struct rstats_t *stats)
{
    if (stats == NULL)
        return;

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

int
stats_update_buf(struct rstats_t *stats, const ldns_buffer *buffer)
{
    if (stats == NULL)
        return STATS_NULL;
    if (buffer == NULL)
        return BUFFER_NULL;

    int err_code = 0;
    
    ldns_pkt *reply = NULL;

    ldns_buffer2pkt_wire(&reply, buffer);
    
    if (stats_update_pkt(stats, reply) < 0)
        err_code = UPDATE_PKT_ERROR;

    ldns_pkt_free(reply);

    return err_code;
}

int
stats_update_pkt(struct rstats_t *stats, const ldns_pkt *pkt)
{
    /* TODO: too crappy code has to be changed */
    switch (ldns_pkt_get_rcode(pkt)) {
        case LDNS_RCODE_NOERROR:
            if (inc_rsts_fld(stats, &(stats->n_noerr)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_FORMERR:
            if (inc_rsts_fld(stats, &(stats->n_formerr)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_SERVFAIL:
            if (inc_rsts_fld(stats, &(stats->n_servfail)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NXDOMAIN:
            if (inc_rsts_fld(stats, &(stats->n_nxdomain)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NOTIMPL:
            if (inc_rsts_fld(stats, &(stats->n_notimpl)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_REFUSED:
            if (inc_rsts_fld(stats, &(stats->n_refused)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_YXDOMAIN:
            if (inc_rsts_fld(stats, &(stats->n_yxdomain)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_YXRRSET:
            if (inc_rsts_fld(stats, &(stats->n_yxrrset)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NXRRSET:
            if (inc_rsts_fld(stats, &(stats->n_nxrrset)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NOTAUTH:
            if (inc_rsts_fld(stats, &(stats->n_notauth)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NOTZONE:
            if (inc_rsts_fld(stats, &(stats->n_notzone)) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
    }
    return 0;
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

int
inc_rsts_fld(struct rstats_t *stats, size_t *field)
{
    if (stats == NULL)
        return STATS_NULL;
    if (field == NULL)
        return FIELD_NULL;

    if (pthread_mutex_lock(&(stats->lock)) != 0)
        fatal("%s: mutex lock error", __func__);
    
    *field = *field + 1;

    if (pthread_mutex_unlock(&(stats->lock)) != 0)
        fatal("%s: mutex unlock error", __func__);
    
    return 0;
}