#include "phandler.h"
#include "utils.h"

#define RECV_TRIES 5

static struct query_t *
__random_query(struct dnsconfig_t *config)
{
    return &config->queries[randint(config->queries_count)];
}

static struct query_t *
get_random_query(struct dnsconfig_t *config)
{
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

static void
compression_node_free(ldns_rbnode_t *node, void *arg)
{
    (void)arg; /* Yes, dear compiler, it is used */
    ldns_rdf_deep_free((ldns_rdf *)node->key);
    LDNS_FREE(node);
}

static void
query_init_inner_bf(struct dnsconfig_t *config, ldns_buffer *buffer) {
    if (config == NULL || buffer == NULL) return;

    struct query_t *query = get_random_query(config);

    ldns_rr_type qtype = ldns_get_rr_type_by_name(query->type);
    ldns_rdf *dname    = ldns_dname_new_frm_str(query->dname);

    ldns_buffer_clear(buffer);
    // ldns_buffer_set_capacity(buffer, LDNS_MAX_PACKETLEN);
    // ldns_buffer_set_limit(buffer, LDNS_MAX_PACKETLEN);

    ldns_buffer_write_u16 (buffer, ldns_get_random()); /* id                 */
    ldns_buffer_write_u16 (buffer, 0x0120);            /* flags              */
    ldns_buffer_write_u16 (buffer, 1);                 /* queries            */
    ldns_buffer_write_u16 (buffer, 0);                 /* answers            */
	ldns_buffer_write_u16 (buffer, 0);                 /* authority records  */
	ldns_buffer_write_u16 (buffer, 0);                 /* additional records */
    ldns_dname2buffer_wire(buffer, dname);             /* domain name        */
    ldns_buffer_write_u16 (buffer, qtype);             /* query type         */
	ldns_buffer_write_u16 (buffer, LDNS_RR_CLASS_IN);  /* query class        */
}

static void
query_init_inner(struct dnsconfig_t *config, ldns_buffer *buffer)
{
    if (config == NULL || buffer == NULL) return;
    
    ldns_pkt *query_pkt = NULL;

    struct query_t *query = get_random_query(config);

    ldns_rdf *dname    = ldns_dname_new_frm_str(query->dname);
    ldns_rr_type qtype = ldns_get_rr_type_by_name(query->type);
    EXIT_ON_NULL(dname);

    query_pkt = ldns_pkt_query_new(dname, qtype, LDNS_RR_CLASS_IN, LDNS_RD);

    if (ldns_pkt_id(query_pkt) == 0)
        ldns_pkt_set_random_id(query_pkt);

    ldns_buffer_clear(buffer);

    if (ldns_pkt2buffer_wire(buffer, query_pkt) != LDNS_STATUS_OK) {
        log_warn("failed to convert pkt into buffer");
        buffer = NULL;
    }
    ldns_pkt_free(query_pkt);
}

/* creates a query inside of servant's buffer */
int
query_create(struct servant_t *servant)
{
    if (servant->buffer == NULL) {
        return BUFFER_NULL;
    }
    if (servant->server == NULL) {
        return SERVER_NULL;
    }
    if (servant->config == NULL) {
        return CONFIG_NULL;
    }
    
    // query_init_inner_bf(servant->config, servant->buffer);
    query_init_inner(servant->config, servant->buffer);

    return 0;
}

int
perform_query(struct servant_t *servant, sender_func send_query)
{
    if (servant == NULL)  return SERVANT_NULL;
    if (!servant->active) return SERVANT_NON_ACTIVE;

    if (servant->server == NULL) {
        fatal("null pointer at servant server");
    }

    if (servant->buffer == NULL) {
        fatal("null pointer at servant buffer");
    }

    query_create(servant);

    ssize_t sent = 0;

    /* try to transmit all data until it will be correctly sent */
    while (true) {
        sent = send_query(servant->buffer,
            servant->fd, &servant->server->addr, servant->server->len);
        if (sent < 0) {
            /** 
             * FIXME
             * We sent less than 0 bytes. So, it's either a fd is broken or
             * a server is unvailable
             */
            fatal("worker: %d | servant: %d/%s | sent %ld bytes", 
                servant->worker_base->index, servant->index, 
                type2str(servant->type), sent);
        } else {
            break;
        }
    }
    return sent;
}

int
recv_reply(struct servant_t *servant, receiver_func recvr, servant_type_t type) {
    if (servant == NULL)
        return SERVANT_NULL;
    
    if (!servant->active)
        return SERVANT_NON_ACTIVE;

    for (size_t i = 0; i < RECV_TRIES; i++) {
        size_t   answer_size = 0;
        uint8_t *answer      = NULL;

        switch (type) {
            case TCP_TYPE:
                answer = recvr(servant->fd, &answer_size);
                break;
            case UDP_TYPE:
                answer = recvr(servant->fd, &answer_size, NULL, NULL);
                break;
            case CLEANED:
                break;
        }

        if (answer == NULL) {
            continue;
        }

        reply_process(servant, answer, answer_size);
        LDNS_FREE(answer);
        break;
    }
    return 0;
}

void
reply_process(struct servant_t *servant, uint8_t *answer, size_t answer_size)
{
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

    memcpy(servant->buffer->_data, answer, answer_size);
    servant->buffer->_position = answer_size;

    // /* MEMORY LEAK */
    // ldns_buffer_new_frm_data(servant->buffer, answer, answer_size);

    stats_update_buf(stats, servant->buffer);
}