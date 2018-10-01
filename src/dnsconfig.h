#ifndef __DNSCONFIG_H__
#define __DNSCONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>


/* dirty code, it should be moved */
typedef enum {
    UDP_VALID,     /* udp valid DNS request */
    UDP_NONVALID,  /* udp non-valid DNS request */
    TCP_VALID,     /* tcp valid DNS request */
    TCP_NONVALID,  /* tcp non-valid DNS request */
    SHUFFLE        /* any DNS requests */
} request_mode_t;

#include "network.h"
#include "jsmn.h"
#include "phandler.h"


#define START_SIZE           32
#define MAX_NUMBER_OF_TOKENS 100000*2

#define MAX_WCOUNT 10

#define MAX_TTL 864000 /* 10 days */
#define MIN_TTL 5   

/* json configuration file fields */
#define ADDRS_TOK   "addrs"
#define WORKERS_TOK "workers"
#define DOMAINS_TOK "queries"
#define TTL_TOK     "ttl"

/* domain's fields in the configuration file */
#define DOM_NAME    "dname"
#define DOM_BLOCKED "blocked"
#define DOM_TYPE    "type"

#define DNS_PORT 53

#define PKTSIZE 65535

struct _saddr {
    struct sockaddr_storage addr;
    socklen_t len;

    char *repr;
    size_t id;
};

struct dnsconfig_t {
    char *configfile;
    FILE *outstream;

    size_t addrs_count;
    struct _saddr *addrs;  /* addresses of dns servers */
    
    request_mode_t mode;  /* mode for stressing */
    size_t workers_count; /* workers count */

    struct query_t *queries;
    size_t queries_count;

    size_t ttl; /* in seconds; 0 is a special case (endless stressing) */

    /**
     * These options are mutually exclusive.
     * So, further we will check if rps equals 0, then we use ld_lvl,
     * and overwise.
     */
    uint16_t ld_lvl; /* load level. from 1 to 10 */
    uint32_t rps;    /* requests per second */
};

struct dnsconfig_t * dnsconfig_create(void);
void dnsconfig_free(struct dnsconfig_t * config);

bool dnsconfig_rps_enabled(const struct dnsconfig_t *config);
bool dnsconfig_ld_lvl_enabled(const struct dnsconfig_t *config);

const char * parse_sockaddr(struct sockaddr_storage *sock,
    char *host, const in_port_t __port);

int parse_config(struct dnsconfig_t *config, const char *filename);

#endif /* __DNSCONFIG_H__ */