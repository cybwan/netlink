#ifndef __FLB_NLP_H__
#define __FLB_NLP_H__

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

#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>

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
  __u32 flags;
  __u32 scope;
  struct nl_ip_net peer;
  struct nl_ip broadcast;
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

int nl_addr_list(nl_port_mod_t *port, __u8 family);
int nl_neigh_list(nl_port_mod_t *port, __u8 family);
int nl_route_list(nl_port_mod_t *port, __u8 family);
int nl_bridge_list();
int nl_link_list();

int nl_addr_subscribe();
int nl_neigh_subscribe();
int nl_route_subscribe();
int nl_link_subscribe();

bool nl_has_loaded_tc_prog(const char *ifi_name);

bool nl_route_add(const char *dst_str, const char *gw_str);
bool nl_route_del(const char *dst_str);
bool nl_addr_add(const char *addr_str, const char *ifi_name);
bool nl_addr_del(const char *addr_str, const char *ifi_name);
bool nl_fdb_add(const char *mac_addr, const char *ifi_name);
bool nl_fdb_del(const char *mac_addr, const char *ifi_name);
bool nl_neigh_add(const char *ip_addr, const char *ifi_name,
                  const char *mac_addr);
bool nl_neigh_del(const char *ip_addr, const char *ifi_name);
bool nl_vxlan_peer_add(__u32 vxlan_id, const char *peer_ip);
bool nl_vxlan_peer_del(__u32 vxlan_id, const char *peer_ip);

bool nl_link_add(nl_link_t *link, int flags);
bool nl_link_up(int ifi_index);
bool nl_link_down(int ifi_index);
bool nl_link_del(int ifi_index);

int nl_link_get_by_index(int ifi_index, nl_port_mod_t *port);
int nl_link_get_by_name(const char *ifi_name, nl_port_mod_t *port);

static __u8 zero_mac[ETH_ALEN] = {0};

static inline bool is_zero_mac(__u8 mac[ETH_ALEN]) {
  if (memcmp(mac, zero_mac, ETH_ALEN) == 0) {
    return true;
  }
  return false;
}

static inline void parse_rtattr(struct rtattr *tb[], int max,
                                struct rtattr *rta, int len) {
  memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

  while (RTA_OK(rta, len)) {
    if (rta->rta_type <= max) {
      tb[rta->rta_type] = rta;
    }
    rta = RTA_NEXT(rta, len);
  }
}

static inline bool parse_ip(const char *ip_str, struct nl_ip *ip) {
  memset(ip, 0, sizeof(*ip));
  if (strchr(ip_str, '.')) {
    ip->f.v4 = 1;
    if (inet_pton(AF_INET, ip_str, (char *)ip->v4.bytes) <= 0) {
      return false;
    }
  } else if (strchr(ip_str, ':')) {
    ip->f.v6 = 1;
    if (inet_pton(AF_INET6, ip_str, (char *)ip->v6.bytes) <= 0) {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

static inline bool parse_ip_net(const char *ip_net_str,
                                struct nl_ip_net *ip_net) {
  memset(ip_net, 0, sizeof(*ip_net));
  char *mask_str = strchr(ip_net_str, '/');
  if (!mask_str) {
    return false;
  }
  __u8 ip[INET6_ADDRSTRLEN] = {0};
  memcpy(ip, ip_net_str, mask_str - ip_net_str);
  if (strchr((char *)ip, '.')) {
    ip_net->ip.f.v4 = 1;
    if (inet_pton(AF_INET, (char *)ip, (char *)ip_net->ip.v4.bytes) <= 0) {
      return false;
    }
  } else if (strchr((char *)ip, ':')) {
    ip_net->ip.f.v6 = 1;
    if (inet_pton(AF_INET6, (char *)ip, (char *)ip_net->ip.v6.bytes) <= 0) {
      return false;
    }
  } else {
    return false;
  }
  mask_str++;
  ip_net->mask = (__u8)atoi(mask_str);
  return true;
}

static inline bool parse_label_net(const char *ip_net_str,
                                   struct nl_label_net *ip_net) {
  memset(ip_net, 0, sizeof(*ip_net));
  char *label_str = strchr(ip_net_str, ' ');
  if (!label_str) {
    return parse_ip_net(ip_net_str, (struct nl_ip_net *)ip_net);
  }
  __u8 ip[INET6_ADDRSTRLEN] = {0};
  memcpy(ip, ip_net_str, label_str - ip_net_str);
  parse_ip_net((const char *)ip, (struct nl_ip_net *)ip_net);
  label_str++;
  memcpy(ip_net->label, label_str, strlen(label_str));
  return true;
}

static inline int hex_to_bin(__u8 ch) {
  __u8 cu = ch & 0xdf;
  return -1 +
         ((ch - '0' + 1) & (unsigned)((ch - '9' - 1) & ('0' - 1 - ch)) >> 8) +
         ((cu - 'A' + 11) & (unsigned)((cu - 'F' - 1) & ('A' - 1 - cu)) >> 8);
}

static inline bool mac_pton(const char *s, __u8 *mac) {
  int i;

  /* XX:XX:XX:XX:XX:XX */
  if (strlen(s) < 3 * ETH_ALEN - 1)
    return false;

  /* Don't dirty result unless string is valid MAC. */
  for (i = 0; i < ETH_ALEN; i++) {
    if (!isxdigit(s[i * 3]) || !isxdigit(s[i * 3 + 1]))
      return false;
    if (i != ETH_ALEN - 1 && s[i * 3 + 2] != ':')
      return false;
  }
  for (i = 0; i < ETH_ALEN; i++) {
    mac[i] = (hex_to_bin(s[i * 3]) << 4) | hex_to_bin(s[i * 3 + 1]);
  }
  return true;
}

#endif /* __FLB_NLP_H__ */