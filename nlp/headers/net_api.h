#ifndef __FLB_NET_API_H__
#define __FLB_NET_API_H__

#include <arpa/inet.h>
#include <linux/types.h>
#include <net/if.h>
#include <stdbool.h>

#ifndef _NL_IP_T_
#define _NL_IP_T_
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
#endif

#ifndef _NL_IPNET_T_
#define _NL_IPNET_T_
typedef struct nl_ipnet {
  struct nl_ip ip;
  __u8 mask;
} nl_ipnet_t;
#endif

struct net_api_port_q {
  __u8 dev[IF_NAMESIZE];
  __u32 link_index;
  __u32 link_type;
  __u8 mac_addr[6];
  bool link;  // link - lowerlayer state
  bool state; // state - administrative state
  __u32 mtu;
  __u8 master[IF_NAMESIZE];
  __u8 real[IF_NAMESIZE];
  __u32 tun_id;
  __u32 tun_src;
  __u32 tun_dst;
};

struct net_api_vlan_q {
  __u32 vid;
  __u8 dev[IF_NAMESIZE];
  __u32 link_index;
  __u8 mac_addr[6];
  bool link;
  bool state;
  __u32 mtu;
  __u32 tun_id;
};

struct net_api_vlan_port_q {
  __u32 vid;
  __u8 dev[IF_NAMESIZE];
  bool tagged;
};

struct net_api_neigh_q {
  __u32 ip;
  __u32 link_index;
  __u32 state;
  __u8 hwaddr[6];
};

struct net_api_fdb_q {
  __u8 mac_addr[6];
  __u32 bridge_id;
  __u32 dst;
  __u32 type;
  __u8 dev[IF_NAMESIZE];
};

struct net_api_addr_q {
  __u8 dev[IF_NAMESIZE];
  __u8 ip[INET6_ADDRSTRLEN + 4];
};

struct net_api_route_q {
  __u32 link_index;
  __u32 protocol;
  __u32 flags;
  struct nl_ip gw;
  struct nl_ipnet dst;
};

int net_port_add(struct net_api_port_q *port);
int net_port_del(struct net_api_port_q *port);
int net_vlan_add(struct net_api_vlan_q *vlan);
int net_vlan_del(struct net_api_vlan_q *vlan);
int net_vlan_port_add(struct net_api_vlan_port_q *vlan_port);
int net_vlan_port_del(struct net_api_vlan_port_q *vlan_port);
int net_neigh_add(struct net_api_neigh_q *neigh);
int net_neigh_del(struct net_api_neigh_q *neigh);
int net_fdb_add(struct net_api_fdb_q *fdb);
int net_fdb_del(struct net_api_fdb_q *fdb);
int net_addr_add(struct net_api_addr_q *addr);
int net_addr_del(struct net_api_addr_q *addr);
int net_route_add(struct net_api_route_q *route);
int net_route_del(struct net_api_route_q *route);

void apply_config_map(const char *name, bool state, bool add);
#endif /* __FLB_NET_API_H__ */