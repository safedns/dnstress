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

typedef enum {
    BUFFER_NULL        = -1,
    SERVER_NULL        = -2,
    CONFIG_NULL        = -3,
    SERVANT_NULL       = -4,
    SERVANT_NON_ACTIVE = -5,
    ANSWER_NULL        = -6
} phandler_error_t;

typedef ssize_t (*sender_func)(ldns_buffer *qbin, int sockfd, 
    const struct sockaddr_storage *to, socklen_t tolen);

typedef uint8_t * (*receiver_func)();

int  query_create(struct servant_t *servant);
void reply_process(struct servant_t *servant, uint8_t *answer, size_t answer_size);
int  perform_query(struct servant_t *servant, sender_func send_query);
int  recv_reply(struct servant_t *servant, receiver_func recvr, servant_type_t type);

#endif /* __QUERY_H__ */