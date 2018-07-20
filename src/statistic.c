#include "statistic.h"
#include "utils.h"

struct rstats_t * stats_create(void) {
    struct rstats_t *stats = (struct rstats_t *) xmalloc_0(sizeof(struct rstats_t));
    return stats;
}

void stats_free(struct rstats_t *stats) {
    free(stats);
}

void stats_update_buf(struct rstats_t *stats, const ldns_buffer *buffer) {
    ldns_pkt *reply = ldns_pkt_new();
    ldns_buffer2pkt_wire(&reply, buffer);
    stats_update_pkt(stats, reply);
}

void stats_update_pkt(struct rstats_t *stats, const ldns_pkt *pkt) {
    return;
}