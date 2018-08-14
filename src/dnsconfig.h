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

#include "jsmn.h"
#include "phandler.h"

#define START_SIZE           32
#define MAX_NUMBER_OF_TOKENS 100000*2

#define MAX_WCOUNT 10

/* json configuration file fields */
#define ADDRS_TOK   "addrs"
#define WORKERS_TOK "workers"
#define DOMAINS_TOK "queries"

/* domain's fields in the configuration file */
#define DOM_NAME    "dname"
#define DOM_BLOCKED "blocked"
#define DOM_TYPE    "type"

#define DNS_PORT 53

#define PKTSIZE 65535

struct _saddr {
    struct sockaddr_storage addr;
    char *repr;
    socklen_t len;
};

struct dnsconfig_t {
    char *configfile;

    size_t addrs_count;
    struct _saddr *addrs;  /* addresses of dns servers */
    
    request_mode_t mode;  /* mode for stressing */
    size_t workers_count; /* workers count */

    struct query_t *queries;
    size_t queries_count;
};

struct dnsconfig_t * dnsconfig_create(void);
void dnsconfig_free(struct dnsconfig_t * config);

int parse_config(struct dnsconfig_t *config, char *filename);

#endif /* __DNSCONFIG_H__ */