#include "tcp.h"
#include "servant.h"

void send_tcp_query() {

}

void recv_tcp_reply(evutil_socket_t fd, short events, void *arg) {
    struct servant_t *servant = (struct servant_t *) arg;
}