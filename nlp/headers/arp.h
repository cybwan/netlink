#ifndef __FLB_ARP_H__
#define __FLB_ARP_H__

#include <linux/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include <arpa/inet.h>

#include <log/log.h>
#include <nlp.h>

/* ARP protocol opcodes. */
#define ARPOP_REQUEST 1 /* ARP request.  */
#define ARPOP_REPLY 2   /* ARP reply.  */

typedef struct arp_msg {
  /* ARP Header */
  __u16 ar_hrd; /* Format of hardware address.  */
  __u16 ar_pro; /* Format of protocol address.  */

  __u8 ar_hln; /* Length of hardware address.  */
  __u8 ar_pln; /* Length of protocol address.  */

  __u16 ar_op; /* ARP opcode (command).  */

  __u8 ar_sha[ETH_ALEN]; /* Sender hardware address.  */
  __u8 ar_sip[IP4_ALEN]; /* Sender IP address.  */
  __u8 ar_tha[ETH_ALEN]; /* Target hardware address.  */
  __u8 ar_tip[IP4_ALEN]; /* Target IP address.  */
} flb_arp_msg_t;

int flb_arp_grat(ip_t *adv_addr, const char *ifname);
int flb_arp_grat_addr(const char *adv_ipv4, const char *ifname);

int flb_arp_ping(ip_t *dst_addr, ip_t *src_addr, const char *ifname);
int flb_arp_ping_addr(const char *dst_ipv4, const char *src_ipv4,
                      const char *ifname);

#endif /* __FLB_ARP_H__ */