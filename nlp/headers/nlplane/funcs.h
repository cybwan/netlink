#ifndef __FLB_NLPLANE_FUNCS_H__
#define __FLB_NLPLANE_FUNCS_H__

struct rtattr;
struct nl_ip;
struct nl_ip_net;
struct nl_link;
struct nl_port_mod;
struct nl_addr_mod;

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
struct nl_addr_mod *nl_link_addr_list(__u32 ifa_index, __u8 family, int *addrs_cnt);

int nl_link_get_by_index(int ifi_index, struct nl_port_mod *port);
int nl_link_get_by_name(const char *ifi_name, struct nl_port_mod *port);

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

#endif /* __FLB_NLPLANE_FUNCS_H__ */