#ifndef __QUERY_H__
#define __QUERY_H__

#include <stdio.h>

#include <ldns/ldns.h>

#include "dnsconfig.h"
#include "servant.h"

struct servant_t;
struct dnsconfig_t;

struct query_t {
    char *dname;
    int blocked;
    char *type;
};

void query_create(struct servant_t *servant);
void reply_process(struct servant_t *servant, uint8_t *answer, size_t answer_size);

#endif /* __QUERY_H__ */