#ifndef __FLB_NLP_H__
#define __FLB_NLP_H__

#include <linux/types.h>

#include <stdio.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_tunnel.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

typedef struct nl_port_mod {
	__u32	index;
  __u32 master_index;
	__u32 flags;
  __u32 mtu;
  __u8  oper_state;
  __u8  hwaddr[6];
  struct {
    __u32 dummy:1;
    __u32 ifb:1;
    __u32 bridge:1;
    __u32 vlan:1;
    __u32 veth:1;
    __u32 wireguard:1;
    __u32 vxlan:1;
    __u32 bond:1;
    __u32 ipvlan:1;
    __u32 macvlan:1;
    __u32 macvtap:1;
    __u32 geneve:1;
    __u32 gretap:1;
    __u32 ip6gretap:1;
    __u32 ipip:1;
    __u32 ip6tnl:1;
    __u32 sit:1;
    __u32 gre:1;
    __u32 ip6gre:1;
    __u32 vti:1;
    __u32 vti6:1;
    __u32 vrf:1;
    __u32 gtp:1;
    __u32 xfrm:1;
    __u32 tun:1;
    __u32 ipoib:1;
    __u32 can:1;
  } type;
  union
  {
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
  __u8  name[IF_NAMESIZE];
} nl_port_mod_t;

typedef struct nl_vlan_mod {
} nl_vlan_mod_t;

typedef struct nl_vlan_port_mod {
} nl_vlan_port_mod_t;

typedef struct nl_ip_addr_mod {
} nl_ip_addr_mod_t;

typedef struct nl_neigh_mod {
} nl_neigh_mod_t;

typedef struct nl_route_mod {
} nl_route_mod_t;

typedef struct nl_fdb_mod {
} nl_fdb_mod_t;

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);

#endif /* __FLB_NLP_H__ */