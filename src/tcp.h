#ifndef __TCP_H__
#define __TCP_H__

#include <event2/event.h>

#include "servant.h"

/* takes into account a sent packet in stats */
void send_tcp_query(struct servant_t *servant);

/* takes into account a received packet in stats */
void recv_tcp_reply(evutil_socket_t fd, short events, void *arg);

#endif /* __TCP_H__ */