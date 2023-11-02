#ifndef __FLB_NLPLANE_TYPES_H__
#define __FLB_NLPLANE_TYPES_H__

#include <ctype.h>
#include <linux/types.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <net/if.h>

#include <linux/if_addr.h>
#include <linux/if_ether.h>
#include <linux/if_tunnel.h>
#include <linux/neighbour.h>
#include <linux/rtnetlink.h>

#include <netlink-generic.h>
#include <netlink-types.h>

#include <attr.h>

#define FAMILY_ALL AF_UNSPEC
#define FAMILY_V4 AF_INET
#define FAMILY_V6 AF_INET6
#define FAMILY_MPLS AF_MPLS

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

#ifndef _NL_IP_NET_T_
#define _NL_IP_NET_T_
typedef struct nl_ip_net {
  struct nl_ip ip;
  __u8 mask;
} nl_ip_net_t;
#endif

#ifndef _NL_LABEL_NET_T_
#define _NL_LABEL_NET_T_
typedef struct nl_label_net {
  struct nl_ip ip;
  __u8 mask;
  __u8 label[IF_NAMESIZE];
} nl_label_net_t;
#endif

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
  __u8 hwaddr[ETH_ALEN];
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
  __u8 name[IF_NAMESIZE];
} nl_port_mod_t;

typedef struct nl_neigh_mod {
  __u32 link_index;
  __u32 family;
  __u32 state;
  __u32 type;
  __u32 flags;
  __u32 vlan;
  __u32 vni;
  __u32 master_index;
  struct nl_ip ip;
  struct nl_ip ll_ip_addr;
  __u8 hwaddr[ETH_ALEN];
} nl_neigh_mod_t;

typedef struct nl_addr_mod {
  struct nl_ip_net ipnet;
  __u8 label[IF_NAMESIZE];
  __u32 flags;
  __u32 scope;
  struct nl_ip_net peer;
  struct nl_ip broadcast;
  __u32 prefered_lft;
  __u32 valid_lft;
  __u32 link_index;
} nl_addr_mod_t;

typedef struct nl_route_mod {
  __u32 link_index;
  __u32 protocol;
  __u32 flags;
  struct nl_ip gw;
  struct nl_ip_net dst;
} nl_route_mod_t;

#define CLS_BPF_NAME_LEN 256
#define BPF_TAG_SIZE 8

typedef struct nl_filter_mod {
  __u32 index;
  __u32 handle;
  __u32 parent;
  __u16 priority;
  __u16 Protocol;
  struct {
    __u32 u32 : 1;
    __u32 fw : 1;
    __u32 bpf : 1;
    __u32 matchall : 1;
    __u32 generic_filter : 1;
  } type;
  union {
    struct {
    } u32;
    struct {
    } fw;
    struct {
      __u32 class_id;
      __u32 fd;
      __u8 name[CLS_BPF_NAME_LEN];
      bool direct_action;
      __u32 id;
      __u8 tag[BPF_TAG_SIZE * 2 + 1];
    } bpf;
    struct {
    } matchall;
    struct {
    } generic_filter;
  } u;
} nl_filter_mod_t;

#endif /* __FLB_NLPLANE_TYPES_H__ */