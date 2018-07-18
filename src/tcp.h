#ifndef __TCP_H__
#define __TCP_H__

#include <event2/event.h>

void send_tcp_query();
void recv_tcp_reply(evutil_socket_t fd, short events, void *arg);

#endif /* __TCP_H__ */