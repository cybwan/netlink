#ifndef __FLB_NLP_H__
#define __FLB_NLP_H__

#include <linux/types.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <net/if.h>

#include <linux/if_tunnel.h>
#include <linux/neighbour.h>

#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>

enum {
  FLAG_UP = 1,       // interface is administratively up
  FLAG_BROADCAST,    // interface supports broadcast access capability
  FLAG_LOOPBACK,     // interface is a loopback interface
  FLAG_POINTTOPOINT, // interface belongs to a point-to-point link
  FLAG_MULTICAST,    // interface supports multicast access capability
  FLAG_RUNNING,      // interface is in running state
};

enum {
  OPER_UNKNOWN,          // Status can't be determined.
  OPER_NOT_PRESENT,      // Some component is missing.
  OPER_DOWN,             // Down.
  OPER_LOWER_LAYER_DOWN, // Down due to state of lower layer.
  OPER_TESTING,          // In some test mode.
  OPER_DORMANT,          // Not up but pending an external event.
  OPER_UP,               // Up, ready to send packets.
};

enum {
  PORT_REAL = 1,       // Base port type
  PORT_BOND_SLAVE_IF,  // Bond slave port type
  PORT_BOND,           // Bond port type
  PORT_VLAN_SLAVE_IF,  // Vlan slave port type
  PORT_VLAN_BR,        // Vlan Br port type
  PORT_VXLAN_SLAVE_IF, // Vxlan slave port type
  PORT_VXLAN_BR,       // Vxlan br port type
  PORT_WG,             // Wireguard port type
  PORT_VTI,            // Vti port type
  PORT_IPTUN,          // IPInIP port type
  PORT_GRE,            // GRE port type
};

enum {
  FDB_PHY,  // fdb of a real dev
  FDB_TUN,  // fdb of a tun dev
  FDB_VLAN, // fdb of a vlan dev
};

typedef struct nl_multi_arg {
  void *arg1;
  void *arg2;
  void *arg3;
} nl_multi_arg_t;

typedef struct nl_port_mod {
  __u32 index;
  __u32 master_index;
  __u32 flags;
  __u32 mtu;
  __u8 oper_state;
  __u8 hwaddr[6];
  struct {
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
  } type;
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
  __u8 name[IF_NAMESIZE];
} nl_port_mod_t;

typedef struct nl_vlan_mod {
} nl_vlan_mod_t;

typedef struct nl_vlan_port_mod {
} nl_vlan_port_mod_t;

typedef struct nl_ip_addr_mod {
} nl_ip_addr_mod_t;

typedef struct nl_neigh_mod {
  __u32 link_index;
  __u32 family;
  __u32 state;
  __u32 type;
  __u32 flags;
  __u32 vlan;
  __u32 vni;
  __u32 master_index;
  __u32 ip;
  __u32 ll_ip_addr;
  __u8 hwaddr[6];
} nl_neigh_mod_t;

typedef struct nl_route_mod {
} nl_route_mod_t;

typedef struct nl_fdb_mod {
} nl_fdb_mod_t;

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);

int nl_link_get(int ifi_index, nl_port_mod_t *port);

int nl_neigh_list(nl_port_mod_t *port, __u8 family);

static __u8 zero_mac[6] = {0, 0, 0, 0, 0, 0};

static inline bool is_zero_mac(__u8 mac[6]) {
  if (memcmp(mac, zero_mac, 6) == 0) {
    return true;
  }
  return false;
}

#endif /* __FLB_NLP_H__ */