#ifndef __FLB_ATTR_H__
#define __FLB_ATTR_H__

#include <linux/if_arp.h>

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
  __u32 can : 1;
} nl_link_type_t;

typedef struct nl_link {
  nl_base_attrs_t attrs;
  nl_link_type_t type;
  union {
    struct {

    } bridge;
    struct {

    } bond;
    struct {
      __u32 vxlan_id;
      __u32 vtep_dev_index;
    } vxlan;
    struct {
      __u32 local;
      __u32 remote;
    } iptun;
  } u;
} nl_link_t;

static inline char *link_type(nl_link_type_t *type) {
  if (type->bond) {
    return "bond";
  } else if (type->bridge) {
    return "bridge";
  }
  return NULL;
}

static const char *STR_ARPHRD_GENERIC = "generic";
static const char *STR_ARPHRD_ETHER = "ether";
static const char *STR_ARPHRD_EETHER = "eether";
static const char *STR_ARPHRD_AX25 = "ax25";
static const char *STR_ARPHRD_PRONET = "pronet";
static const char *STR_ARPHRD_CHAOS = "chaos";
static const char *STR_ARPHRD_IEEE802 = "ieee802";
static const char *STR_ARPHRD_ARCNET = "arcnet";
static const char *STR_ARPHRD_APPLETLK = "atalk";
static const char *STR_ARPHRD_DLCI = "dlci";
static const char *STR_ARPHRD_ATM = "atm";
static const char *STR_ARPHRD_METRICOM = "metricom";
static const char *STR_ARPHRD_IEEE1394 = "ieee1394";
static const char *STR_ARPHRD_INFINIBAND = "infiniband";
static const char *STR_ARPHRD_SLIP = "slip";
static const char *STR_ARPHRD_CSLIP = "cslip";
static const char *STR_ARPHRD_SLIP6 = "slip6";
static const char *STR_ARPHRD_CSLIP6 = "cslip6";
static const char *STR_ARPHRD_RSRVD = "rsrvd";
static const char *STR_ARPHRD_ADAPT = "adapt";
static const char *STR_ARPHRD_ROSE = "rose";
static const char *STR_ARPHRD_X25 = "x25";
static const char *STR_ARPHRD_HWX25 = "hwx25";
static const char *STR_ARPHRD_PPP = "ppp";
static const char *STR_ARPHRD_HDLC = "hdlc";
static const char *STR_ARPHRD_LAPB = "lapb";
static const char *STR_ARPHRD_DDCMP = "ddcmp";
static const char *STR_ARPHRD_RAWHDLC = "rawhdlc";
static const char *STR_ARPHRD_TUNNEL = "ipip";
static const char *STR_ARPHRD_TUNNEL6 = "tunnel6";
static const char *STR_ARPHRD_FRAD = "frad";
static const char *STR_ARPHRD_SKIP = "skip";
static const char *STR_ARPHRD_LOOPBACK = "loopback";
static const char *STR_ARPHRD_LOCALTLK = "ltalk";
static const char *STR_ARPHRD_FDDI = "fddi";
static const char *STR_ARPHRD_BIF = "bif";
static const char *STR_ARPHRD_SIT = "sit";
static const char *STR_ARPHRD_IPDDP = "ip/ddp";
static const char *STR_ARPHRD_IPGRE = "gre";
static const char *STR_ARPHRD_PIMREG = "pimreg";
static const char *STR_ARPHRD_HIPPI = "hippi";
static const char *STR_ARPHRD_ASH = "ash";
static const char *STR_ARPHRD_ECONET = "econet";
static const char *STR_ARPHRD_IRDA = "irda";
static const char *STR_ARPHRD_FCPP = "fcpp";
static const char *STR_ARPHRD_FCAL = "fcal";
static const char *STR_ARPHRD_FCPL = "fcpl";
static const char *STR_ARPHRD_FCFABRIC = "fcfb0";
static const char *STR_ARPHRD_FCFABRIC1 = "fcfb1";
static const char *STR_ARPHRD_FCFABRIC2 = "fcfb2";
static const char *STR_ARPHRD_FCFABRIC3 = "fcfb3";
static const char *STR_ARPHRD_FCFABRIC4 = "fcfb4";
static const char *STR_ARPHRD_FCFABRIC5 = "fcfb5";
static const char *STR_ARPHRD_FCFABRIC6 = "fcfb6";
static const char *STR_ARPHRD_FCFABRIC7 = "fcfb7";
static const char *STR_ARPHRD_FCFABRIC8 = "fcfb8";
static const char *STR_ARPHRD_FCFABRIC9 = "fcfb9";
static const char *STR_ARPHRD_FCFABRIC10 = "fcfb10";
static const char *STR_ARPHRD_FCFABRIC11 = "fcfb11";
static const char *STR_ARPHRD_FCFABRIC12 = "fcfb12";
static const char *STR_ARPHRD_IEEE802_TR = "tr";
static const char *STR_ARPHRD_IEEE80211 = "ieee802.11";
static const char *STR_ARPHRD_IEEE80211_PRISM = "ieee802.11/prism";
static const char *STR_ARPHRD_IEEE80211_RADIOTAP = "ieee802.11/radiotap";
static const char *STR_ARPHRD_IEEE802154 = "ieee802.15.4";
static const char *STR_ARPHRD_65534 = "none";
static const char *STR_ARPHRD_65535 = "void";

static inline char *encap_type(__u16 type) {
  switch (type) {
  case 0:
    return (char *)STR_ARPHRD_GENERIC;
  case ARPHRD_ETHER:
    return (char *)STR_ARPHRD_ETHER;
  case ARPHRD_EETHER:
    return (char *)STR_ARPHRD_EETHER;
  case ARPHRD_AX25:
    return (char *)STR_ARPHRD_AX25;
  case ARPHRD_PRONET:
    return (char *)STR_ARPHRD_PRONET;
  case ARPHRD_CHAOS:
    return (char *)STR_ARPHRD_CHAOS;
  case ARPHRD_IEEE802:
    return (char *)STR_ARPHRD_IEEE802;
  case ARPHRD_ARCNET:
    return (char *)STR_ARPHRD_ARCNET;
  case ARPHRD_APPLETLK:
    return (char *)STR_ARPHRD_APPLETLK;
  case ARPHRD_DLCI:
    return (char *)STR_ARPHRD_DLCI;
  case ARPHRD_ATM:
    return (char *)STR_ARPHRD_ATM;
  case ARPHRD_METRICOM:
    return (char *)STR_ARPHRD_METRICOM;
  case ARPHRD_IEEE1394:
    return (char *)STR_ARPHRD_IEEE1394;
  case ARPHRD_INFINIBAND:
    return (char *)STR_ARPHRD_INFINIBAND;
  case ARPHRD_SLIP:
    return (char *)STR_ARPHRD_SLIP;
  case ARPHRD_CSLIP:
    return (char *)STR_ARPHRD_CSLIP;
  case ARPHRD_SLIP6:
    return (char *)STR_ARPHRD_SLIP6;
  case ARPHRD_CSLIP6:
    return (char *)STR_ARPHRD_CSLIP6;
  case ARPHRD_RSRVD:
    return (char *)STR_ARPHRD_RSRVD;
  case ARPHRD_ADAPT:
    return (char *)STR_ARPHRD_ADAPT;
  case ARPHRD_ROSE:
    return (char *)STR_ARPHRD_ROSE;
  case ARPHRD_X25:
    return (char *)STR_ARPHRD_X25;
  case ARPHRD_HWX25:
    return (char *)STR_ARPHRD_HWX25;
  case ARPHRD_PPP:
    return (char *)STR_ARPHRD_PPP;
  case ARPHRD_HDLC:
    return (char *)STR_ARPHRD_HDLC;
  case ARPHRD_LAPB:
    return (char *)STR_ARPHRD_LAPB;
  case ARPHRD_DDCMP:
    return (char *)STR_ARPHRD_DDCMP;
  case ARPHRD_RAWHDLC:
    return (char *)STR_ARPHRD_RAWHDLC;
  case ARPHRD_TUNNEL:
    return (char *)STR_ARPHRD_TUNNEL;
  case ARPHRD_TUNNEL6:
    return (char *)STR_ARPHRD_TUNNEL6;
  case ARPHRD_FRAD:
    return (char *)STR_ARPHRD_FRAD;
  case ARPHRD_SKIP:
    return (char *)STR_ARPHRD_SKIP;
  case ARPHRD_LOOPBACK:
    return (char *)STR_ARPHRD_LOOPBACK;
  case ARPHRD_LOCALTLK:
    return (char *)STR_ARPHRD_LOCALTLK;
  case ARPHRD_FDDI:
    return (char *)STR_ARPHRD_FDDI;
  case ARPHRD_BIF:
    return (char *)STR_ARPHRD_BIF;
  case ARPHRD_SIT:
    return (char *)STR_ARPHRD_SIT;
  case ARPHRD_IPDDP:
    return (char *)STR_ARPHRD_IPDDP;
  case ARPHRD_IPGRE:
    return (char *)STR_ARPHRD_IPGRE;
  case ARPHRD_PIMREG:
    return (char *)STR_ARPHRD_PIMREG;
  case ARPHRD_HIPPI:
    return (char *)STR_ARPHRD_HIPPI;
  case ARPHRD_ASH:
    return (char *)STR_ARPHRD_ASH;
  case ARPHRD_ECONET:
    return (char *)STR_ARPHRD_ECONET;
  case ARPHRD_IRDA:
    return (char *)STR_ARPHRD_IRDA;
  case ARPHRD_FCPP:
    return (char *)STR_ARPHRD_FCPP;
  case ARPHRD_FCAL:
    return (char *)STR_ARPHRD_FCAL;
  case ARPHRD_FCPL:
    return (char *)STR_ARPHRD_FCPL;
  case ARPHRD_FCFABRIC:
    return (char *)STR_ARPHRD_FCFABRIC;
  case ARPHRD_FCFABRIC + 1:
    return (char *)STR_ARPHRD_FCFABRIC1;
  case ARPHRD_FCFABRIC + 2:
    return (char *)STR_ARPHRD_FCFABRIC2;
  case ARPHRD_FCFABRIC + 3:
    return (char *)STR_ARPHRD_FCFABRIC3;
  case ARPHRD_FCFABRIC + 4:
    return (char *)STR_ARPHRD_FCFABRIC4;
  case ARPHRD_FCFABRIC + 5:
    return (char *)STR_ARPHRD_FCFABRIC5;
  case ARPHRD_FCFABRIC + 6:
    return (char *)STR_ARPHRD_FCFABRIC6;
  case ARPHRD_FCFABRIC + 7:
    return (char *)STR_ARPHRD_FCFABRIC7;
  case ARPHRD_FCFABRIC + 8:
    return (char *)STR_ARPHRD_FCFABRIC8;
  case ARPHRD_FCFABRIC + 9:
    return (char *)STR_ARPHRD_FCFABRIC9;
  case ARPHRD_FCFABRIC + 10:
    return (char *)STR_ARPHRD_FCFABRIC10;
  case ARPHRD_FCFABRIC + 11:
    return (char *)STR_ARPHRD_FCFABRIC11;
  case ARPHRD_FCFABRIC + 12:
    return (char *)STR_ARPHRD_FCFABRIC12;
  case ARPHRD_IEEE802_TR:
    return (char *)STR_ARPHRD_IEEE802_TR;
  case ARPHRD_IEEE80211:
    return (char *)STR_ARPHRD_IEEE80211;
  case ARPHRD_IEEE80211_PRISM:
    return (char *)STR_ARPHRD_IEEE80211_PRISM;
  case ARPHRD_IEEE80211_RADIOTAP:
    return (char *)STR_ARPHRD_IEEE80211_RADIOTAP;
  case ARPHRD_IEEE802154:
    return (char *)STR_ARPHRD_IEEE802154;
  case 65534:
    return (char *)STR_ARPHRD_65534;
  case 65535:
    return (char *)STR_ARPHRD_65535;
  default:
    return NULL;
  }
}

#endif