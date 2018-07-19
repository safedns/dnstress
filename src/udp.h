#ifndef __UDP_H__
#define __UDP_H__

#include <event2/event.h>

void send_udp_query(struct servant_t *servant);
void recv_udp_reply(evutil_socket_t fd, short events, void *arg);

#endif /* __UDP_H__ */