#ifndef __DNSCONFIG_H__
#define __DNSCONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "jsmn.h"

typedef struct dnsconfig_t dnsconfig_t;

typedef enum {
    LOW_VALID,     /* low sized valid DNS request */
    LOW_NONVALID,  /* high sized valid DNS request */
    HIGH_VALID,    /* low sized non-valid DNS request */
    HIGH_NONVALID, /* high sized non-valid DNS request */
    SHUFFLE        /* any DNS requests */
} gen_mode_t;

struct dnsconfig_t {
    char *configfile;

    struct sockaddr_storage *addrs;  /* addresses of dns servers */
    gen_mode_t mode;                 /* mode for stressing */
    size_t     wcount;               /* workers count */
};

int get_pair(jsmntok_t *tokens, char *content, size_t index, char *key, char *value);
int parse_config(dnsconfig_t *config, char *filename);

#endif /* __DNSCONFIG_H__ */