#ifndef __DNSCONFIG_H__
#define __DNSCONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "jsmn.h"

#define START_SIZE           32
#define MAX_NUMBER_OF_TOKENS 100000*2

/* json configuration file fields */
#define ADDRS_TOK "addrs"

typedef struct dnsconfig_t dnsconfig_t;

typedef enum {
    LOW_VALID,     /* low sized valid DNS request */
    LOW_NONVALID,  /* high sized valid DNS request */
    HIGH_VALID,    /* low sized non-valid DNS request */
    HIGH_NONVALID, /* high sized non-valid DNS request */
    SHUFFLE        /* any DNS requests */
} request_mode_t;

struct dnsconfig_t {
    char *configfile;

    size_t addrs_count;
    struct sockaddr_storage *addrs;  /* addresses of dns servers */
    
    request_mode_t mode;  /* mode for stressing */
    size_t wcount;        /* workers count */
};

dnsconfig_t * dnsconfig_create(void);
void          dnsconfig_free(dnsconfig_t * config);

int parse_config(dnsconfig_t *config, char *filename);

#endif /* __DNSCONFIG_H__ */