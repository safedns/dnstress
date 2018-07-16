#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <stdio.h>
#include <stdlib.h>

#include "dnsconfig.h"

#define HELP "--help"

/* TODO: think about modes a little bit more */
#define MODE       "--mode"
#define L_VALID    "low-valid"      /* low sized valid DNS request */
#define H_VALID    "high-valid"     /* high sized valid DNS request */
#define L_NONVALID "low-nonvalid"   /* low sized non-valid DNS request */
#define H_NONVALID "high-nonvalid"  /* high sized non-valid DNS request */
#define _SHUFFLE   "shuffle"        /* any DNS requests */

#define WCOUNT        "--wcount"    /* workers count */
#define DEFAULT_COUNT 20            /* default workers count */
#define MIN_WCOUNT    1

#define CONF "--conf"               /* file with dnstress configuration */

typedef struct m_entity_t m_entity_t;

struct m_entity_t {
    char * key;
    size_t value;
};

void usage(void);
void parse_args(int argc, char **argv, dnsconfig_t *config);

#endif