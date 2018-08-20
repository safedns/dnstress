#include "statistic.h"
#include "utils.h"

struct rstats_t *
stats_create(struct dnsconfig_t *config)
{
    if (config == NULL)
        fatal("%s: null pointer to config", __func__);

    struct rstats_t     *stats      = xmalloc_0(sizeof(struct rstats_t));
    struct serv_stats_t *serv_stats = xmalloc_0(sizeof(struct serv_stats_t) * config->addrs_count);

    if (stats == NULL)
        fatal("%s: failed to malloc stats", __func__);
    if (serv_stats == NULL)
        fatal("%s: failed to malloc serv stats", __func__);

    if (pthread_mutex_init(&stats->lock, NULL) != 0)
        goto err_ret;

    if (pthread_cond_init(&stats->cond, NULL) != 0)
        goto err_ret;

    for (size_t i = 0; i < config->addrs_count; i++) {
        serv_stats[i].server = &config->addrs[i];
        serv_stats[i].lock   = &stats->lock;
        serv_stats[i].cond   = &stats->cond;
    }

    stats->servs       = serv_stats;
    stats->servs_count = config->addrs_count;
    stats->config      = config;
    stats->tmp_struct  = false;

    return stats;

err_ret:
    free(stats);
    free(serv_stats);
    
    return NULL;
}

struct rstats_t *
stats_create_tmp(void)
{
    struct rstats_t *stats = xmalloc_0(sizeof(struct rstats_t));

    if (stats == NULL)
        fatal("%s: failed to malloc tmp stats", __func__);

    stats->tmp_struct = true;

    return stats;
}

void
stats_free(struct rstats_t *stats)
{
    if (stats == NULL)
        return;

    if (!stats->tmp_struct) {
        pthread_mutex_destroy(&stats->lock);
        pthread_cond_destroy(&stats->cond);
    }

    if (stats->servs) {
        for (size_t i = 0; i < stats->servs_count; i++) {
            stats->servs[i].server = NULL;
            stats->servs[i].lock   = NULL;
            stats->servs[i].cond   = NULL;
        }
        free(stats->servs);
    }
    
    stats->servs  = NULL;
    stats->config = NULL;
    
    free(stats);
}

static struct serv_stats_t *
get_serv_stats_addr(const struct rstats_t *stats, const struct _saddr *addr)
{
    for (size_t i = 0; i < stats->servs_count; i++) {
        if (stats->servs[i].server->id == addr->id)
            return &stats->servs[i];
    }
    log_warn("%s: serv_stats wasn't found", __func__);
    return NULL;
}

/* debugging function */
void
__stats_update_servant(struct rstats_t *stats,
    const struct servant_t *servant, const servant_type_t conn_type)
{

    ldns_pkt *reply = NULL;

    if (ldns_buffer2pkt_wire(&reply, servant->buffer) != LDNS_STATUS_OK)
        fatal("%s: failed to convert buffer to pkt wire", __func__);
    
    if (ldns_pkt_get_rcode(reply) == LDNS_RCODE_SERVFAIL)
        log_warn("worker: %d | servant: %d | servfail!",
            servant->worker_base->index, servant->index);    

    stats_update_pkt(get_serv_stats_addr(stats, servant->server),
        reply, conn_type);
    ldns_pkt_free(reply);
}

static int
update_stats_entity(struct stats_entity_t *entity1,
    const struct stats_entity_t *entity2)
{
    if (entity1 == NULL || entity2 == NULL)
        return ENTITY_NULL;

    entity1->n_sent += entity2->n_sent;
    entity1->n_recv += entity2->n_recv;

    entity1->n_noerr    += entity2->n_noerr;
    entity1->n_formerr  += entity2->n_formerr;
    entity1->n_servfail += entity2->n_servfail;
    entity1->n_nxdomain += entity2->n_nxdomain;
    entity1->n_notimpl  += entity2->n_notimpl;
    entity1->n_refused  += entity2->n_refused;
    entity1->n_yxdomain += entity2->n_yxdomain;
    entity1->n_yxrrset  += entity2->n_yxrrset;
    entity1->n_nxrrset  += entity2->n_nxrrset;
    entity1->n_notauth  += entity2->n_notauth;
    entity1->n_notzone  += entity2->n_notzone;

    entity1->n_corrupted += entity2->n_corrupted;
    
    return 0;
}

static struct stats_entity_t *
get_entity(struct serv_stats_t *sv_stats, const servant_type_t conn_type)
{
    struct stats_entity_t *entity   = NULL;

    switch (conn_type) {
        case UDP_TYPE:
            entity = &sv_stats->udp_serv;
            break;
        case TCP_TYPE:
            entity = &sv_stats->tcp_serv;
            break;
        default:
            fatal("%s: unknown connection type", __func__);
            break;
    }
    return entity;
}

int
stats_update_buf(struct rstats_t *stats, const struct _saddr *server,
    const ldns_buffer *buffer, const servant_type_t conn_type)
{
    if (stats == NULL)
        return STATS_NULL;
    if (stats->servs == NULL)
        return SERV_STATS_NULL;
    if (server == NULL)
        return ADDR_NULL;
    if (buffer == NULL)
        return BUFFER_NULL;

    int err_code    = 0;
    ldns_pkt *reply = NULL;

    struct serv_stats_t   *sv_stats = get_serv_stats_addr(stats, server);
    struct stats_entity_t *entity   = get_entity(sv_stats, conn_type);

    if (sv_stats == NULL) {
        log_warn("%s: failed to get serv_stats", __func__);
        return SERV_STATS_NULL;
    }

    if (entity == NULL) {
        log_warn("%s: failed to init stats entity", __func__);
        return ENTITY_NULL;
    }

    if (ldns_buffer2pkt_wire(&reply, buffer) != LDNS_STATUS_OK) {
        if (inc_rsts_fld(sv_stats, &entity->n_corrupted) < 0) {
            fatal("%s: failed to increment n_corrupted field", __func__);
        }
        return 0;
    }

    if (stats_update_pkt(sv_stats, reply, conn_type) < 0)
        err_code = UPDATE_PKT_ERROR;

    ldns_pkt_free(reply);

    return err_code;
}

int
stats_update_pkt(struct serv_stats_t *sv_stats, const ldns_pkt *pkt, 
    const servant_type_t conn_type)
{
    struct stats_entity_t *entity = get_entity(sv_stats, conn_type);
    
    /* TODO: too crappy code has to be changed */
    switch (ldns_pkt_get_rcode(pkt)) {
        case LDNS_RCODE_NOERROR:
            if (inc_rsts_fld(sv_stats, &entity->n_noerr) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_FORMERR:
            if (inc_rsts_fld(sv_stats, &entity->n_formerr) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_SERVFAIL:
            if (inc_rsts_fld(sv_stats, &entity->n_servfail) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NXDOMAIN:
            if (inc_rsts_fld(sv_stats, &entity->n_nxdomain) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NOTIMPL:
            if (inc_rsts_fld(sv_stats, &entity->n_notimpl) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_REFUSED:
            if (inc_rsts_fld(sv_stats, &entity->n_refused) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_YXDOMAIN:
            if (inc_rsts_fld(sv_stats, &entity->n_yxdomain) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_YXRRSET:
            if (inc_rsts_fld(sv_stats, &entity->n_yxrrset) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NXRRSET:
            if (inc_rsts_fld(sv_stats, &entity->n_nxrrset) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NOTAUTH:
            if (inc_rsts_fld(sv_stats, &entity->n_notauth) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
        case LDNS_RCODE_NOTZONE:
            if (inc_rsts_fld(sv_stats, &entity->n_notzone) < 0)
                return INC_RSTS_FLD_ERROR;
            break;
    }
    return 0;
}

void
stats_update_stats(struct rstats_t *stats1, const struct rstats_t *stats2)
{
    if (pthread_mutex_lock(&stats1->lock) != 0)
        fatal("%s: mutex lock error", __func__);

    for (size_t i = 0; i < stats1->servs_count; i++) {
        struct serv_stats_t *sv_stats1 = &stats1->servs[i];
        struct serv_stats_t *sv_stats2 = get_serv_stats_addr(stats2, sv_stats1->server);

        if (sv_stats2 == NULL)
            fatal("%s: failed to get serv stats by addr", __func__);

        if (update_stats_entity(&sv_stats1->tcp_serv,
            &sv_stats2->tcp_serv) < 0)
            fatal("%s: failed to update tcp stats entity", __func__);
        if (update_stats_entity(&sv_stats1->udp_serv,
            &sv_stats2->udp_serv) < 0)
            fatal("%s: failed to update udp stats entity", __func__);

        stats1->__call_num++;
    }

    if (pthread_mutex_unlock(&stats1->lock) != 0)
        fatal("%s: mutex unlock error", __func__);
}

struct serv_stats_t *
get_serv_stats_servant(struct servant_t *servant)
{
    struct rstats_t *stats = gstats(servant);
    struct serv_stats_t *sv_stats = get_serv_stats_addr(stats, servant->server);

    return sv_stats;
}

static size_t
calc_errs(const struct stats_entity_t *entity)
{
    return  entity->n_formerr +
            entity->n_servfail +
            entity->n_nxdomain +
            entity->n_notimpl +
            entity->n_refused +
            entity->n_yxdomain +
            entity->n_yxrrset +
            entity->n_nxrrset +
            entity->n_notauth +
            entity->n_notzone +
            entity->n_corrupted;
}

static float
perc(size_t first, size_t second)
{
    if (first == 0)
        return 0;
    if (second == 0)
        return 0;
    return (float) first / second * 100;
}

void
print_stats(struct rstats_t *stats)
{
    char *stick = NULL;
    char *angle = NULL;
    
    fprintf(stderr, "\n");
    fprintf(stderr, "   STRESS STATISTIC\n\n");
    
    for (size_t i = 0; i < stats->servs_count; i++) {
        if (i < stats->servs_count - 1) {
            stick = "│";
            angle = "├";
        } else {
            stick = " ";
            angle = "└";
        }
        
        struct serv_stats_t   *sv_stats = &stats->servs[i];
        
        struct stats_entity_t *tcp_ent  = get_entity(sv_stats, TCP_TYPE);
        struct stats_entity_t *udp_ent  = get_entity(sv_stats, UDP_TYPE);

        const char *addr_repr = sv_stats->server->repr;

        if (sv_stats == NULL)
            fatal("%s: null pointer to serv stats", __func__);
        if (tcp_ent == NULL)
            fatal("%s: failed to get tcp entity", __func__);
        if (udp_ent == NULL)
            fatal("%s: failed to get udp entity", __func__);

        float udp_tr_loss = 100.0 - perc(udp_ent->n_recv, udp_ent->n_sent);
        float udp_rc_loss = perc(calc_errs(udp_ent), udp_ent->n_noerr);
        
        float tcp_tr_loss = 100.0 - perc(tcp_ent->n_recv, tcp_ent->n_sent);
        float tcp_rc_loss = perc(calc_errs(tcp_ent), tcp_ent->n_noerr);

        fprintf(stderr, "    %s─ %s\n", angle, addr_repr); // address representation
        fprintf(stderr, "    %s    ├─── UDP\n", stick);
        fprintf(stderr, "    %s    │    ├── TRANSMITTING INFO\n", stick);
        fprintf(stderr, "    %s    │    │   ├─ queries:   %zu\n", stick, udp_ent->n_sent);
        fprintf(stderr, "    %s    │    │   ├─ responses: %zu\n", stick, udp_ent->n_recv);
        fprintf(stderr, "    %s    │    │   └─ loss:      %.2f%%\n", stick, udp_tr_loss);
        fprintf(stderr, "    %s    │    │          \n", stick);
        fprintf(stderr, "    %s    │    └── RESPONSE CODES\n", stick);
        fprintf(stderr, "    %s    │        ├─ noerr:     %zu\n",stick, udp_ent->n_noerr);
        fprintf(stderr, "    %s    │        ├─ corrupted: %zu\n",stick, udp_ent->n_corrupted);
        fprintf(stderr, "    %s    │        ├─ formaerr:  %zu\n", stick, udp_ent->n_formerr);
        fprintf(stderr, "    %s    │        ├─ servfail:  %zu\n", stick, udp_ent->n_servfail);
        fprintf(stderr, "    %s    │        └─ errors:    %.2f%%\n", stick, udp_rc_loss);
        fprintf(stderr, "    %s    │               \n", stick);
        fprintf(stderr, "    %s    └─── TCP\n", stick);
        fprintf(stderr, "    %s         ├── TRANSMITTING INFO\n", stick);
        fprintf(stderr, "    %s         │   ├─ queries:   %zu\n", stick, tcp_ent->n_sent);
        fprintf(stderr, "    %s         │   ├─ responses: %zu\n", stick, tcp_ent->n_recv);
        fprintf(stderr, "    %s         │   └─ loss:      %.2f%%\n", stick, tcp_tr_loss);
        fprintf(stderr, "    %s         │          \n", stick);
        fprintf(stderr, "    %s         └── RESPONSE CODES\n", stick);
        fprintf(stderr, "    %s             ├─ noerr:     %zu\n", stick, tcp_ent->n_noerr);
        fprintf(stderr, "    %s             ├─ corrupted: %zu\n", stick, tcp_ent->n_corrupted);
        fprintf(stderr, "    %s             ├─ formaerr:  %zu\n", stick, tcp_ent->n_formerr);
        fprintf(stderr, "    %s             ├─ servfail:  %zu\n", stick, tcp_ent->n_servfail);
        fprintf(stderr, "    %s             └─ errors:    %.2f%%\n", stick, tcp_rc_loss);
        fprintf(stderr, "    %s                   \n", stick);
        // fprintf(stderr, "      ==== ERRORS ====\n");
        // fprintf(stderr, "        [+] NO ERRORS:       %zu\n", stats->n_noerr);
        // fprintf(stderr, "        [-] CORRUPTED:       %zu\n", stats->n_corrupted);
        // fprintf(stderr, "        [-] FORMAT ERROR:    %zu\n", stats->n_formerr);
        // fprintf(stderr, "        [-] SERVER FAILURE:  %zu\n", stats->n_servfail);
        // fprintf(stderr, "        [-] NAME ERROR:      %zu\n", stats->n_nxdomain);
        // fprintf(stderr, "        [-] NOT IMPLEMENTED: %zu\n", stats->n_notimpl);
        // fprintf(stderr, "        [-] REFUSED:         %zu\n", stats->n_refused);
    }
    fprintf(stderr, "\n");
}

int
inc_rsts_fld(struct serv_stats_t *sv_stats, size_t *field)
{
    if (sv_stats == NULL)
        return STATS_NULL;
    if (field == NULL)
        return FIELD_NULL;

    if (pthread_mutex_lock(sv_stats->lock) != 0)
        fatal("%s: mutex lock error", __func__);
    
    *field = *field + 1;

    if (pthread_mutex_unlock(sv_stats->lock) != 0)
        fatal("%s: mutex unlock error", __func__);
    
    return 0;
}