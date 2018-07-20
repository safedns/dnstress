#ifndef __STATISTIC_H__
#define __STATISTIC_H__

#include <stdio.h>

#include <ldns/ldns.h>

struct rstats_t {
    size_t sent_pkts;
    size_t recv_pkts;

    size_t err_pkts;
};

struct rstats_t * stats_create(void);
void stats_free(struct rstats_t * stats);

void stats_update_buf(struct rstats_t *stats, const ldns_buffer *buffer);
void stats_update_pkt(struct rstats_t *stats, const ldns_pkt *pkt);

#endif /* __STATISTIC_H__ */