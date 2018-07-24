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

void query_create(struct dnsconfig_t * config, ldns_buffer *buffer) {
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

void reply_process(ldns_buffer *buffer) {
    /* FIXME: useless?? */
}