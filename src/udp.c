#include "udp.h"
#include "network.h"
#include "servant.h"
#include "utils.h"

static struct domain_t * __random_domain(struct dnsconfig_t *config) {
    return &config->domains[randint(config->domains_count)];
}

static struct domain_t * get_random_domain(struct dnsconfig_t *config) {
    struct domain_t *domain = NULL;
    switch (config->mode) {
        case TCP_VALID:
        case UDP_VALID:
            while ((domain = __random_domain(config))->blocked == 1)
                continue;
            break;
        case UDP_NONVALID:
        case TCP_NONVALID:
            while ((domain = __random_domain(config))->blocked == 0)
                continue;
            break;
        case SHUFFLE:
            domain = __random_domain(config);
            break;
    }
    return domain;
}

static ldns_buffer * prepare_query(struct servant_t *servant) {
    struct dnsconfig_t *config = servant->config;
    struct domain_t    *domain = get_random_domain(config);
    
    ldns_buffer *buffer = ldns_buffer_new(PKTSIZE);
    EXIT_ON_NULL(buffer);

    ldns_rdf *dname = ldns_dname_new_frm_str(domain->name);
    EXIT_ON_NULL(dname);

    int qtype       = ldns_get_rr_type_by_name(domain->type);
    int qclass      = LDNS_RR_CLASS_IN;
    
    ldns_buffer_clear(buffer);

    

    return NULL;
}

void send_udp_query(struct servant_t *servant) {
//     struct iovec vec[1];

//     ldns_buffer * query = prepare_query(servant);

// 	vec[0].iov_base = ldns_buffer_begin(query);
// 	vec[0].iov_len  = ldns_buffer_limit(query);

// /* TODO IPv6: change to sockaddr_in6 for IPv6 processing */
// 	struct sockaddr_in dest; /* TODO IPv6: sockaddr_in -> sockaddr_in6 */
// 	memset(&dest, 0, sizeof(dest));
// 	dest.sin_family = AF_INET;
// 	dest.sin_addr.s_addr = pkt->src_addr;
// 	dest.sin_port = pkt->src_port;

// 	struct msghdr header;
// 	header.msg_name = &dest;
// 	header.msg_namelen = sizeof(dest);
// 	header.msg_iov    = vec;
// 	header.msg_iovlen = 1;
// 	header.msg_flags  = 0;

// /**
//  * TODO IPv6: add checking for IPV6_PKTINFO
//  * 
//  * IPV6_PKTINFO is implemented on the most systems including
//  * Linux and FreeBSD, so this don't need to check other cases
//  * like IP_SENDSRCADDR in IPv4 case
//  */
// #if defined(IP_PKTINFO)
// 	uint8_t cmsgbuf[CMSG_SPACE(sizeof(struct in_pktinfo))];
// 	memset(cmsgbuf, 0, sizeof(cmsgbuf));

// 	header.msg_control    = cmsgbuf;
// 	header.msg_controllen = sizeof(cmsgbuf);

// /* TODO IPv6: add or change this bunch of code for supporting IPv6 */
// 	struct cmsghdr* ch = CMSG_FIRSTHDR(&header);
// 	ch->cmsg_len   = CMSG_LEN(sizeof(struct in_pktinfo)); /* TODO IPv6: sizeof(struct in6_pktinfo) */
// 	ch->cmsg_level = IPPROTO_IP;                          /* TODO IPv6: IPPROTO_IPV6 */
// 	ch->cmsg_type  = IP_PKTINFO;                          /* TODO IPv6: IPV6_PKTINFO*/
	
// /* TODO IPv6: use in6_pktinfo for IPv6 processing */
// 	struct in_pktinfo* pktinfo = (struct in_pktinfo*) CMSG_DATA(ch);
// 	pktinfo->ipi_ifindex = 0;
// 	pktinfo->ipi_spec_dst.s_addr = pkt->dst_addr;
// 	pktinfo->ipi_addr.s_addr = 0;

// #elif defined(IP_SENDSRCADDR)
// 	uint8_t cmsgbuf[CMSG_SPACE(sizeof(struct in_addr))];
// 	memset(cmsgbuf, 0, sizeof(cmsgbuf));
	
// 	header.msg_control    = cmsgbuf;
// 	header.msg_controllen = sizeof(cmsgbuf);

// 	struct cmsghdr* ch = CMSG_FIRSTHDR(&header);
// 	ch->cmsg_len   = CMSG_LEN(sizeof(struct in_addr));
// 	ch->cmsg_level = IPPROTO_IP;
// 	ch->cmsg_type  = IP_SENDSRCADDR;

// 	struct in_addr* addr = (struct in_addr*)CMSG_DATA(ch);
// 	addr->s_addr = pkt->dst_addr;
// #else
// 	header.msg_control    = NULL;
// 	header.msg_controllen = 0;
// #endif

// 	sendmsg(pkt->reply_fd, &header, 0);
}

void recv_udp_reply(evutil_socket_t fd, short events, void *arg) {
    struct servant_t *servant = (struct servant_t *) arg;
}