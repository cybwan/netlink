#ifndef __FLB_NLPLANE_FUNCS_H__
#define __FLB_NLPLANE_FUNCS_H__

#include <linux/rtnetlink.h>

#include <nlplane/types.h>

int nl_addr_list(struct nl_port_mod *port, __u8 family);
int nl_neigh_list(struct nl_port_mod *port, __u8 family);
int nl_route_list(struct nl_port_mod *port, __u8 family);
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

bool nl_vlan_add(int vlan_id);
bool nl_vlan_del(int vlan_id);
bool nl_vlan_member_add(int vlan_id, const char *ifi_name, bool tagged);
bool nl_vlan_member_del(int vlan_id, const char *ifi_name, bool tagged);

bool nl_vxlan_bridge_add(int vxlan_id, const char *ep_ifi_name);
bool nl_vxlan_del(int vxlan_id);

bool nl_link_add(struct nl_link *link, int flags);
bool nl_link_up(int ifi_index);
bool nl_link_down(int ifi_index);
bool nl_link_del(int ifi_index);
bool nl_link_master(int ifi_index, int master_ifi_index);
bool nl_link_no_master(int ifi_index);
struct nl_addr_mod *nl_link_addr_list(__u32 ifa_index, __u8 family,
                                      int *addrs_cnt);

int nl_link_get_by_index(int ifi_index, struct nl_port_mod *port);
int nl_link_get_by_name(const char *ifi_name, struct nl_port_mod *port);

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

#endif /* __FLB_NLPLANE_FUNCS_H__ */