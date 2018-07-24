#ifndef __STATISTIC_H__
#define __STATISTIC_H__

#include <stdio.h>

#include <ldns/ldns.h>
#include "servant.h"

struct servant_t;

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
};

struct rstats_t * stats_create(void);
void stats_free(struct rstats_t * stats);

/* prints to log if servant gets failed response */
void __stats_update_servant(struct rstats_t *stats, struct servant_t *servant);
void stats_update_buf(struct rstats_t *stats, const ldns_buffer *buffer);
void stats_update_pkt(struct rstats_t *stats, const ldns_pkt *pkt);

void print_stats(struct rstats_t *stats);

#endif /* __STATISTIC_H__ */