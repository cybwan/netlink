#ifndef __FLB_NET_API_H__
#define __FLB_NET_API_H__

#include <linux/types.h>
#include <net/if.h>
#include <stdbool.h>

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

int net_port_add(struct net_api_port_q *port);
int net_port_del(struct net_api_port_q *port);
int net_vlan_add(struct net_api_vlan_q *vlan);
int net_vlan_del(struct net_api_vlan_q *vlan);
int net_vlan_port_add(struct net_api_vlan_port_q *vlan_port);
int net_vlan_port_del(struct net_api_vlan_port_q *vlan_port);

void apply_config_map(const char *name, bool state, bool add);
#endif /* __FLB_NET_API_H__ */