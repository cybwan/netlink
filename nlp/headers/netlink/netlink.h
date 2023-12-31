/*
 * netlink/netlink.h		Netlink Interface
 */

#ifndef NETLINK_NETLINK_H_
#define NETLINK_NETLINK_H_

#include <linux/genetlink.h>
#include <linux/netlink.h>
#include <netdb.h>
#include <netlink/errno.h>
#include <netlink/handlers.h>
#include <netlink/netlink-compat.h>
#include <netlink/socket.h>
#include <netlink/types.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int nl_debug;
extern struct nl_dump_params nl_debug_dp;

/* Connection Management */
extern int nl_connect(struct nl_sock *, int);
extern void nl_close(struct nl_sock *);

/* Send */
extern int nl_sendto(struct nl_sock *, void *, size_t);
extern int nl_sendmsg(struct nl_sock *, struct nl_msg *, struct msghdr *);
extern int nl_send(struct nl_sock *, struct nl_msg *);
extern int nl_send_auto_complete(struct nl_sock *, struct nl_msg *);
extern int nl_send_simple(struct nl_sock *, int, int, void *, size_t);

/* Receive */
extern int nl_recv(struct nl_sock *, struct sockaddr_nl *, unsigned char **,
                   struct ucred **);
extern int nl_recvmsgs(struct nl_sock *sk, struct nl_cb *cb);

extern int nl_wait_for_ack(struct nl_sock *);

/* Netlink Family Translations */
extern char *nl_nlfamily2str(int, char *, size_t);
extern int nl_str2nlfamily(const char *);

/**
 * Receive a set of message from a netlink socket using handlers in nl_sock.
 * @arg sk		Netlink socket.
 *
 * Calls nl_recvmsgs() with the handlers configured in the netlink socket.
 */
static inline int nl_recvmsgs_default(struct nl_sock *sk) {
  return nl_recvmsgs(sk, sk->s_cb);
}

#ifdef __cplusplus
}
#endif

#endif
