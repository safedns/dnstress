#include <alloca.h>

#include "proc.h"
#include "utils.h"
#include "log.h"

/**
 * Data for transmition is represented like that:
 * 
 * +------+--------+------------+------------+-----+
 * | size | rstats | sv_stats-1 | sv_stats-1 | ... |
 * +------+--------+------------+------------+-----+
 * 
 * /size       - overall size of a tranmittied data
 * /rstats     - rstats_t object
 * /sv_stats-i - i-th serv_stats_t object
*/

static size_t
calc_data_size(const struct rstats_t *stats)
{
    size_t trans_size = 0;

    trans_size += sizeof(size_t); /* size_t size */
    trans_size += sizeof(*stats); /* rstats_t size */
    
    for (size_t i = 0; i < stats->servs_count; i++) {
        trans_size += sizeof(stats->servs[i]); /* serv_stats_t size */
    }

    return trans_size;
}

/**
 * Transmits rstats_t object to a proc_fd in the format
 * specified above.
 */
ssize_t
proc_transmit_rstats(int proc_fd, const struct rstats_t *stats)
{
    size_t trans_size = calc_data_size(stats);
    size_t written    = 0;

    log_info("%s: size of transmitted data: %zu", __func__, trans_size);

    void * mem = xmalloc_0(trans_size);

    memcpy(mem + written, &trans_size, sizeof(size_t)); /* size */
    written += sizeof(size_t);

    memcpy(mem + written, stats, sizeof(*stats)); /* rstats_t */
    written += sizeof(*stats);

    for (size_t i = 0; i < stats->servs_count; i++) {
        const struct serv_stats_t *sv_stats = &stats->servs[i];
        
        memcpy(mem + written, sv_stats, sizeof(*sv_stats)); /* serv_stats_t */
        written += sizeof(*sv_stats);
    }

    assert(written == trans_size);

    ssize_t wrote = write(proc_fd, mem, trans_size);
    
    free(mem);

    return wrote;
}

/**
 * Obtains data from proc_fd and converts it to a rstats_t object
*/
ssize_t proc_obtain_rstats(int proc_fd, struct rstats_t *stats)
{
    /**
     * save some rstats_t fields to restore them
     * after obtaining rstats_t object from proc_fd
    */
    pthread_mutex_t *lock = &stats->lock;
    pthread_cond_t  *cond = &stats->cond;
    
    struct dnsconfig_t *config = stats->config;
    struct serv_stats_t *servs = stats->servs;

    struct _saddr **servers = alloca(sizeof(stats->servs[0].server) * stats->servs_count);
    
    size_t trans_size = 0;
    size_t obtained   = 0;

    ssize_t recv = 0;

    /* save servers of each serv_stats_t object */
    for (size_t i = 0; i < stats->servs_count; i++)
        servers[i] = stats->servs[i].server;
    
    recv = read(proc_fd, &trans_size, sizeof(size_t));
    if (recv < 0) {
        fatal("%s: failed to obtain trans_size from proc_fd", __func__);
        return recv;
    }
    obtained += recv;
    
    recv = read(proc_fd, stats, sizeof(struct rstats_t));
    if (recv < 0) {
        fatal("%s: failed to obtain rstats_t object", __func__);
        return recv;
    }
    obtained += recv;

    /* restore fields */
    stats->lock   = *lock;
    stats->cond   = *cond;
    stats->servs  = servs;
    stats->config = config;

    for (size_t i = 0; ; i++) {
        if (obtained >= trans_size)
            break;
        
        struct serv_stats_t *sv_stats = &stats->servs[i];

        recv = read(proc_fd, sv_stats, sizeof(*sv_stats));
        if (recv < 0) {
            fatal("%s: failed to obtain sv_stats", __func__);
            return recv;
        }
        obtained += recv;

        sv_stats->cond   = cond;
        sv_stats->lock   = lock;
        sv_stats->server = servers[i];
    }

    assert(obtained == trans_size);

    return obtained;
}