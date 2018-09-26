#ifndef __STATISTIC_H__
#define __STATISTIC_H__

#include <stdio.h>
#include <pthread.h>

#include <ldns/ldns.h>

#include "servant.h"
#include "dnsconfig.h"

struct servant_t;

typedef enum {
    STATS_NULL         = -1,
    PKT_NULL           = -2,
    FIELD_NULL         = -3,
    INC_RSTS_FLD_ERROR = -4,
    UPDATE_PKT_ERROR   = -5,
    UPDATE_BUF_ERROR   = -6,
    SERV_STATS_NULL    = -7,
    ADDR_NULL          = -8,
    ENTITY_NULL        = -9
} stats_error_t;

struct stats_entity_t {
    size_t n_sent;
    size_t n_recv;

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
    
    size_t n_corrupted;
};

/* represents statistic for each server */
struct serv_stats_t {
    struct _saddr *server;
    
    struct stats_entity_t tcp_serv;
    struct stats_entity_t udp_serv;

    pthread_mutex_t *lock;
    pthread_cond_t  *cond;
};

struct rstats_t {
    struct serv_stats_t *servs;
    size_t servs_count;

    pthread_mutex_t lock;
    pthread_cond_t  cond;

    struct dnsconfig_t *config;

    size_t __call_num;

    bool tmp_struct;
};

struct rstats_t * stats_create(struct dnsconfig_t *config);
struct rstats_t * stats_create_tmp(void);
void stats_free(struct rstats_t * stats);

/* prints to log if servant gets failed response */
void __stats_update_servant(struct rstats_t *stats, 
    const struct servant_t *servant, const servant_type_t conn_type);

/* refactor these functions, cause arguments are too crapy */
int stats_update_buf(struct rstats_t *stats, const struct _saddr *addr, 
    const ldns_buffer *buffer, const servant_type_t conn_type);
int stats_update_pkt(struct serv_stats_t *sv_stats, const ldns_pkt *pkt, 
    const servant_type_t conn_type);

void stats_update_stats(struct rstats_t *stats1, const struct rstats_t *stats2);

struct serv_stats_t * get_serv_stats_servant(const struct servant_t *servant);

void print_stats(const struct rstats_t *stats);

int inc_rsts_fld(const struct serv_stats_t *stats, size_t *field);

#endif /* __STATISTIC_H__ */