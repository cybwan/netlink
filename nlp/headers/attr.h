#ifndef __FLB_ATTR_H__
#define __FLB_ATTR_H__

#include <linux/if_arp.h>

#define RTA_PADDING(rta) RTA_ALIGN(sizeof(rta)) - sizeof(rta)

typedef struct nl_ip {
  struct {
    __u8 v4 : 1;
    __u8 v6 : 1;
  } f;
  union {
    union {
      __u8 bytes[16];
    } v6;
    union {
      __u8 _pad[12];
      union {
        __u8 bytes[4];
        __u32 ip;
      };
    } v4;
  };
} nl_ip_t;

typedef struct nl_link_statistics_64 {
  __u64 rx_packets;
  __u64 tx_packets;
  __u64 rx_bytes;
  __u64 tx_bytes;
  __u64 rx_errors;
  __u64 tx_errors;
  __u64 rx_dropped;
  __u64 tx_dropped;
  __u64 multicast;
  __u64 collisions;
  __u64 rx_lengtherrors;
  __u64 rx_overerrors;
  __u64 rx_crcerrors;
  __u64 rx_frameerrors;
  __u64 rx_fifoerrors;
  __u64 rx_missederrors;
  __u64 tx_abortederrors;
  __u64 tx_carriererrors;
  __u64 tx_fifoerrors;
  __u64 tx_heartbeaterrors;
  __u64 tx_windowerrors;
  __u64 rx_compressed;
  __u64 tx_compressed;
} nl_link_statistics_t;

typedef struct nl_link_xdp {
  __s32 fd;
  bool attached;
  __u32 flags;
  __u32 prog_id;
} nl_link_xdp_t;

// nl_link_prot_info represents bridge flags from netlink.
typedef struct nl_link_prot_info {
  bool hairpin;
  bool guard;
  bool fastleave;
  bool rootblock;
  bool learning;
  bool flood;
  bool proxy_arp;
  bool proxy_arp_wifi;
} nl_link_prot_info_t;

typedef struct nl_vf_info {
  __s32 id;
  __u8 mac[ETH_ALEN];
  __s32 vlan;
  __s32 qos;
  __s32 tx_rate;
  bool spoof_chk;
  __u32 link_state;
  __u32 max_tx_rate;
  __u32 min_tx_rate;
} nl_vf_info_t;

typedef struct nl_namespace {
  struct {
    __u8 ns_pid : 1;
    __u8 ns_fd : 1;
  } type;
  __u32 ns;
} nl_namespace_t;

typedef struct nl_base_attrs {
  __s32 index;
  __s32 mtu;
  __s32 tx_q_len; // Transmit Queue Length
  char *name;
  __u8 hw_addr[ETH_ALEN];
  __u32 flags;
  __u32 raw_flags;
  __s32 parent_index;
  __s32 master_index;
  nl_namespace_t *namespace;
  char *alias;
  nl_link_statistics_t *link_statistics;
  __s32 promisc;
  nl_link_xdp_t *xdp;
  __u8 *encap_type;
  nl_link_prot_info_t *prot_info;
  __u8 link_oper_state;
  __s32 netns_id;
  __s32 num_tx_queues;
  __s32 num_rx_queues;
  __u32 gso_max_size;
  __u32 gso_max_segs;
  nl_vf_info_t *vfs;
  __u32 group;
  void *slave;

} nl_base_attrs_t;

typedef struct nl_link_type {
  __u32 dummy : 1;
  __u32 ifb : 1;
  __u32 bridge : 1;
  __u32 vlan : 1;
  __u32 veth : 1;
  __u32 wireguard : 1;
  __u32 vxlan : 1;
  __u32 bond : 1;
  __u32 ipvlan : 1;
  __u32 macvlan : 1;
  __u32 macvtap : 1;
  __u32 geneve : 1;
  __u32 gretap : 1;
  __u32 ip6gretap : 1;
  __u32 ipip : 1;
  __u32 ip6tnl : 1;
  __u32 sit : 1;
  __u32 gre : 1;
  __u32 ip6gre : 1;
  __u32 vti : 1;
  __u32 vti6 : 1;
  __u32 vrf : 1;
  __u32 gtp : 1;
  __u32 xfrm : 1;
  __u32 tun : 1;
  __u32 ipoib : 1;
  __u32 vcan : 1;
  __u32 vxcan : 1;
} nl_link_type_t;

typedef struct nl_bond_ad_info {
  __s32 aggregator_id;
  __s32 num_ports;
  __s32 actor_key;
  __s32 partner_key;
  __u8 partner_mac[ETH_ALEN];
} nl_bond_ad_info_t;

typedef struct nl_link {
  nl_base_attrs_t attrs;
  nl_link_type_t type;
  union {
    struct {
      bool *multicast_snooping;
      __u32 *hello_time;
      bool *vlan_filtering;
    } bridge;
    struct {
      __s32 mode;
      __s32 activeslave;
      __s32 miimon;
      __s32 up_delay;
      __s32 down_delay;
      __s32 use_carrier;
      __s32 arp_interval;
      nl_ip_t *arp_ip_targets;
      __s32 arp_validate;
      __s32 arp_all_targets;
      __s32 primary;
      __s32 primary_reselect;
      __s32 failover_mac;
      __s32 xmit_hash_policy;
      __s32 resend_igmp;
      __s32 num_peer_notif;
      __s32 all_slaves_active;
      __s32 min_links;
      __s32 lp_interval;
      __s32 packers_per_slave;
      __s32 lacp_rate;
      __s32 ad_select;
      nl_bond_ad_info_t *bond_ad_info;
      __s32 ad_actor_sys_prio;
      __s32 ad_user_port_key;
      __u8 ad_actor_system[ETH_ALEN];
      __s32 tlb_dynamic_lb;
    } bond;
    struct {
      char *peer_name;
      __u8 peer_hw_addr[ETH_ALEN];
    } veth;
    struct {
      __s32 vlan_id;
      __s32 vlan_protocol;
    } vlan;
    struct {
      __s32 vxlan_id;
      __s32 vtep_dev_index;
      nl_ip_t *src_addr;
      nl_ip_t *group;
      __s32 ttl;
      __s32 tos;
      bool learning;
      bool proxy;
      bool rsc;
      bool l2_miss;
      bool l3_miss;
      bool udp_csum;
      bool udp6_zero_csum_tx;
      bool udp6_zero_csum_rx;
      bool no_age;
      bool gbp;
      bool flow_based;
      __s32 age;
      __s32 limit;
      __s32 port;
      __s32 port_low;
      __s32 port_high;
    } vxlan;
    struct {
      __u32 local;
      __u32 remote;
    } iptun;
    struct {
      __u32 ikey;
      __u32 okey;
      __u32 link;
      __u32 local;
      __u32 remote;
    } vti;
    struct {
      __u32 table;
    } vrf;
    struct {
      __u32 fd0;
      __u32 fd1;
      __u32 role;
      __u32 pdp_hash_size;
    } gtp;
    struct {
      __u32 ifid;
    } xfrm;
    struct {
      __u16 pkey;
      __u16 mode;
      __u16 umcast;
    } ipoib;
  } u;
} nl_link_t;

static inline char *link_type(nl_link_type_t *type) {
  if (type->dummy) {
    return "dummy";
  } else if (type->ifb) {
    return "ifb";
  } else if (type->bridge) {
    return "bridge";
  } else if (type->vlan) {
    return "vlan";
  } else if (type->veth) {
    return "veth";
  } else if (type->vxlan) {
    return "vxlan";
  } else if (type->bond) {
    return "bond";
  } else if (type->ipvlan) {
    return "ipvlan";
  } else if (type->macvlan) {
    return "macvlan";
  } else if (type->macvtap) {
    return "macvtap";
  } else if (type->gretap) {
    return "gretap";
  } else if (type->ip6gretap) {
    return "ip6gretap";
  } else if (type->ipip) {
    return "ipip";
  } else if (type->ip6tnl) {
    return "ip6tnl";
  } else if (type->sit) {
    return "sit";
  } else if (type->gre) {
    return "gre";
  } else if (type->ip6gre) {
    return "ip6gre";
  } else if (type->vti) {
    return "vti";
  } else if (type->vti6) {
    return "vti";
  } else if (type->vrf) {
    return "vrf";
  } else if (type->gtp) {
    return "gtp";
  } else if (type->xfrm) {
    return "xfrm";
  }
  return NULL;
}

static inline char *encap_type(__u16 type) {
  switch (type) {
  case 0:
    return "generic";
  case ARPHRD_ETHER:
    return "ether";
  case ARPHRD_EETHER:
    return "eether";
  case ARPHRD_AX25:
    return "ax25";
  case ARPHRD_PRONET:
    return "pronet";
  case ARPHRD_CHAOS:
    return "chaos";
  case ARPHRD_IEEE802:
    return "ieee802";
  case ARPHRD_ARCNET:
    return "arcnet";
  case ARPHRD_APPLETLK:
    return "atalk";
  case ARPHRD_DLCI:
    return "dlci";
  case ARPHRD_ATM:
    return "atm";
  case ARPHRD_METRICOM:
    return "metricom";
  case ARPHRD_IEEE1394:
    return "ieee1394";
  case ARPHRD_INFINIBAND:
    return "infiniband";
  case ARPHRD_SLIP:
    return "slip";
  case ARPHRD_CSLIP:
    return "cslip";
  case ARPHRD_SLIP6:
    return "slip6";
  case ARPHRD_CSLIP6:
    return "cslip6";
  case ARPHRD_RSRVD:
    return "rsrvd";
  case ARPHRD_ADAPT:
    return "adapt";
  case ARPHRD_ROSE:
    return "rose";
  case ARPHRD_X25:
    return "x25";
  case ARPHRD_HWX25:
    return "hwx25";
  case ARPHRD_PPP:
    return "ppp";
  case ARPHRD_HDLC:
    return "hdlc";
  case ARPHRD_LAPB:
    return "lapb";
  case ARPHRD_DDCMP:
    return "ddcmp";
  case ARPHRD_RAWHDLC:
    return "rawhdlc";
  case ARPHRD_TUNNEL:
    return "ipip";
  case ARPHRD_TUNNEL6:
    return "tunnel6";
  case ARPHRD_FRAD:
    return "frad";
  case ARPHRD_SKIP:
    return "skip";
  case ARPHRD_LOOPBACK:
    return "loopback";
  case ARPHRD_LOCALTLK:
    return "ltalk";
  case ARPHRD_FDDI:
    return "fddi";
  case ARPHRD_BIF:
    return "bif";
  case ARPHRD_SIT:
    return "sit";
  case ARPHRD_IPDDP:
    return "ip/ddp";
  case ARPHRD_IPGRE:
    return "gre";
  case ARPHRD_PIMREG:
    return "pimreg";
  case ARPHRD_HIPPI:
    return "hippi";
  case ARPHRD_ASH:
    return "ash";
  case ARPHRD_ECONET:
    return "econet";
  case ARPHRD_IRDA:
    return "irda";
  case ARPHRD_FCPP:
    return "fcpp";
  case ARPHRD_FCAL:
    return "fcal";
  case ARPHRD_FCPL:
    return "fcpl";
  case ARPHRD_FCFABRIC:
    return "fcfb0";
  case ARPHRD_FCFABRIC + 1:
    return "fcfb1";
  case ARPHRD_FCFABRIC + 2:
    return "fcfb2";
  case ARPHRD_FCFABRIC + 3:
    return "fcfb3";
  case ARPHRD_FCFABRIC + 4:
    return "fcfb4";
  case ARPHRD_FCFABRIC + 5:
    return "fcfb5";
  case ARPHRD_FCFABRIC + 6:
    return "fcfb6";
  case ARPHRD_FCFABRIC + 7:
    return "fcfb7";
  case ARPHRD_FCFABRIC + 8:
    return "fcfb8";
  case ARPHRD_FCFABRIC + 9:
    return "fcfb9";
  case ARPHRD_FCFABRIC + 10:
    return "fcfb10";
  case ARPHRD_FCFABRIC + 11:
    return "fcfb11";
  case ARPHRD_FCFABRIC + 12:
    return "fcfb12";
  case ARPHRD_IEEE802_TR:
    return "tr";
  case ARPHRD_IEEE80211:
    return "ieee802.11";
  case ARPHRD_IEEE80211_PRISM:
    return "ieee802.11/prism";
  case ARPHRD_IEEE80211_RADIOTAP:
    return "ieee802.11/radiotap";
  case ARPHRD_IEEE802154:
    return "ieee802.15.4";
  case 65534:
    return "none";
  case 65535:
    return "void";
  default:
    return NULL;
  }
}

#endif