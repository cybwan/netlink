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

typedef struct grat_arp_msg {
  /* ARP Header */
  __u16 hardware_type;
  __u16 protocol_type;

  __u8 hardware_address_length;
  __u8 protocol_address_length;

  __u16 arp_options;

  __u8 src_hardware_address[ETH_ALEN];
  __u8 src_protocol_address[4];

  __u8 tgt_hardware_address[ETH_ALEN];
  __u8 tgt_protocol_address[4];
} grat_arp_msg_t;

int flb_grat_arp_req(const char *adv_ipv4, const char *ifname);

#endif /* __FLB_ARP_H__ */