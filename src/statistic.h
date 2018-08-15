#ifndef __STATISTIC_H__
#define __STATISTIC_H__

#include <stdio.h>
#include <pthread.h>

#include <ldns/ldns.h>
#include "servant.h"

struct servant_t;

typedef enum {
    STATS_NULL         = -1,
    PKT_NULL           = -2,
    FIELD_NULL         = -3,
    INC_RSTS_FLD_ERROR = -4,
    UPDATE_PKT_ERROR   = -5,
    UPDATE_BUF_ERROR   = -6
} stats_error_t;

struct rstats_t {
    size_t n_sent_udp;
    size_t n_recv_udp;

    size_t n_sent_tcp;
    size_t n_recv_tcp;

    size_t n_noerr;
    size_t n_formerr;
    size_t n_servfail;
    size_t n_nxdomain;
    size_t n_notimpl;
    size_t n_refused;
    size_t n_yxdomain;
    size_t n_yxrrset;
    size_t n_nxrrset;
    size_t n_notauth;
    size_t n_notzone;

    pthread_mutex_t lock;
    pthread_cond_t  cond;

    size_t __call_num;
};

struct rstats_t * stats_create(void);
void stats_free(struct rstats_t * stats);

/* prints to log if servant gets failed response */
void __stats_update_servant(struct rstats_t *stats, struct servant_t *servant);
int  stats_update_buf(struct rstats_t *stats, const ldns_buffer *buffer);
int  stats_update_pkt(struct rstats_t *stats, const ldns_pkt *pkt);

void stats_update_stats(struct rstats_t *stats1, const struct rstats_t *stats2);

void print_stats(struct rstats_t *stats);

int  inc_rsts_fld(struct rstats_t *stats, size_t *field);

#endif /* __STATISTIC_H__ */