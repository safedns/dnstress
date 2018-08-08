#include "phandler.h"
#include "utils.h"

static struct query_t * __random_query(struct dnsconfig_t *config) {
    return &config->queries[randint(config->queries_count)];
}

static struct query_t * get_random_query(struct dnsconfig_t *config) {
    struct query_t *query = NULL;
    switch (config->mode) {
        case TCP_VALID:
        case UDP_VALID:
            while ((query = __random_query(config))->blocked == 1)
                continue;
            break;
        case UDP_NONVALID:
        case TCP_NONVALID:
            while ((query = __random_query(config))->blocked == 0)
                continue;
            break;
        case SHUFFLE:
            query = __random_query(config);
            break;
    }
    return query;
}

static void query_create_inner(struct dnsconfig_t *config, ldns_buffer *buffer) {
    if (config == NULL || buffer == NULL) return;
    
    ldns_pkt *query_pkt = NULL;

    struct query_t *query = get_random_query(config);

    ldns_rdf *dname    = ldns_dname_new_frm_str(query->dname);
    ldns_rr_type qtype = ldns_get_rr_type_by_name(query->type);
    EXIT_ON_NULL(dname);

    query_pkt = ldns_pkt_query_new(dname, qtype, LDNS_RR_CLASS_IN, LDNS_RD);

    if (ldns_pkt_id(query_pkt) == 0)
        ldns_pkt_set_random_id(query_pkt);

    if (ldns_pkt2buffer_wire(buffer, query_pkt) != LDNS_STATUS_OK) {
        log_warn("failed to convert pkt into buffer");
        buffer = NULL;
    }

    ldns_pkt_free(query_pkt);
}

void query_create(struct servant_t *servant) {    
    if (servant->buffer == NULL)
        fatal("%s: null pointer at servant buffer", __func__);
    if (servant->server == NULL)
        fatal("%s: null pointer at servant server", __func__);
    if (servant->config == NULL)
        fatal("%s: null pointer at servant server", __func__);
    
    ldns_buffer_clear(servant->buffer);
    
    query_create_inner(servant->config, servant->buffer);
}

void reply_process(struct servant_t *servant, uint8_t *answer, size_t answer_size) {
    if (servant->buffer == NULL)
        fatal("%s: null pointer at servant's buffer", __func__);
    
    struct rstats_t *stats = gstats(servant);

    switch (servant->type) {
        case TCP_TYPE:
            inc_rsts_fld(stats, &(stats->n_recv_tcp));
            break;
        case UDP_TYPE:
            inc_rsts_fld(stats, &(stats->n_recv_udp));
            break;
        default:
            fatal("%s: unknown servant type", __func__);
    }

    ldns_buffer_new_frm_data(servant->buffer, answer, answer_size);

    stats_update_buf(stats, servant->buffer);
}