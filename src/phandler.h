#ifndef __QUERY_H__
#define __QUERY_H__

#include <stdio.h>

#include <ldns/ldns.h>

#include "dnsconfig.h"

struct dnsconfig_t;

struct query_t {
    char *dname;
    int blocked;
    char *type;
};

void query_create(struct dnsconfig_t *config, ldns_buffer *buffer);
void reply_process(ldns_buffer *buffer);

#endif /* __QUERY_H__ */