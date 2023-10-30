#include <linux/rtnetlink.h>
#include <nlp.h>
#include <unistd.h>

bool nl_route_add(const char *dst_str, const char *gw_str) {
  if (strlen(dst_str) == 0 || strlen(gw_str) == 0) {
    return false;
  }
  struct nl_ip_net dst;
  if (!parse_ip_net(dst_str, &dst) || (!dst.ip.f.v4 && !dst.ip.f.v6)) {
    return false;
  }
  struct nl_ip gw;
  if (!parse_ip(gw_str, &gw) || (!gw.f.v4 && !gw.f.v6)) {
    return false;
  }
  if (dst.ip.f.v4 != gw.f.v4 || dst.ip.f.v6 != gw.f.v6) {
    return false;
  }
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  if (dst.ip.f.v4) {
    struct {
      struct rtmsg rtm;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_dst;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_gw;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_oif;
    } * nl_req;

    struct nlmsghdr *nlh =
        nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_NEWROUTE, sizeof(*nl_req),
                  NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);

    nl_req = nlmsg_data(nlh);
    memset(nl_req, 0, sizeof(*nl_req));
    nl_req->rtm.rtm_family = FAMILY_V4;
    nl_req->rtm.rtm_dst_len = dst.mask;
    nl_req->rtm.rtm_table = RT_TABLE_MAIN;
    nl_req->rtm.rtm_scope = RT_SCOPE_UNIVERSE;
    nl_req->rtm.rtm_protocol = RTPROT_BOOT;
    nl_req->rtm.rtm_type = RTN_UNICAST;
    nl_req->rta_dst.rta_type = RTA_DST;
    nl_req->rta_dst.rta_len = 8;
    memcpy(nl_req->rta_dst.rta_val, dst.ip.v4.bytes, 4);
    nl_req->rta_gw.rta_type = RTA_GATEWAY;
    nl_req->rta_gw.rta_len = 8;
    memcpy(nl_req->rta_gw.rta_val, gw.v4.bytes, 4);
    nl_req->rta_oif.rta_type = RTA_OIF;
    nl_req->rta_oif.rta_len = 8;
    nl_req->rta_oif.rta_val = 0;
  } else if (dst.ip.f.v6) {
    struct {
      struct rtmsg rtm;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[16];
      } rta_dst;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[16];
      } rta_gw;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_oif;
    } * nl_req;

    struct nlmsghdr *nlh =
        nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_NEWROUTE, sizeof(*nl_req),
                  NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);

    nl_req = nlmsg_data(nlh);
    memset(nl_req, 0, sizeof(*nl_req));
    nl_req->rtm.rtm_family = FAMILY_V6;
    nl_req->rtm.rtm_dst_len = dst.mask;
    nl_req->rtm.rtm_table = RT_TABLE_MAIN;
    nl_req->rtm.rtm_scope = RT_SCOPE_UNIVERSE;
    nl_req->rtm.rtm_protocol = RTPROT_BOOT;
    nl_req->rtm.rtm_type = RTN_UNICAST;
    nl_req->rta_dst.rta_type = RTA_DST;
    nl_req->rta_dst.rta_len = 20;
    memcpy(nl_req->rta_dst.rta_val, dst.ip.v6.bytes, 16);
    nl_req->rta_gw.rta_type = RTA_GATEWAY;
    nl_req->rta_gw.rta_len = 20;
    memcpy(nl_req->rta_gw.rta_val, gw.v6.bytes, 16);
    nl_req->rta_oif.rta_type = RTA_OIF;
    nl_req->rta_oif.rta_len = 8;
    nl_req->rta_oif.rta_val = 0;
  } else {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  int ret = nl_send_auto_complete(socket, msg);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  nlmsg_free(msg);
  nl_socket_free(socket);

  return true;
}

bool nl_route_del(const char *dst_str) {
  if (strlen(dst_str) == 0) {
    return false;
  }
  struct nl_ip_net dst;
  if (!parse_ip_net(dst_str, &dst) || (!dst.ip.f.v4 && !dst.ip.f.v6)) {
    return false;
  }
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  if (dst.ip.f.v4) {
    struct {
      struct rtmsg rtm;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_dst;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_oif;
    } * nl_req;

    struct nlmsghdr *nlh =
        nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_DELROUTE, sizeof(*nl_req),
                  NLM_F_REQUEST | NLM_F_ACK);

    nl_req = nlmsg_data(nlh);
    memset(nl_req, 0, sizeof(*nl_req));
    nl_req->rtm.rtm_family = FAMILY_V4;
    nl_req->rtm.rtm_dst_len = dst.mask;
    nl_req->rtm.rtm_table = RT_TABLE_MAIN;
    nl_req->rtm.rtm_scope = RT_SCOPE_UNIVERSE;
    nl_req->rta_dst.rta_type = RTA_DST;
    nl_req->rta_dst.rta_len = 8;
    memcpy(nl_req->rta_dst.rta_val, dst.ip.v4.bytes, 4);
    nl_req->rta_oif.rta_type = RTA_OIF;
    nl_req->rta_oif.rta_len = 8;
    nl_req->rta_oif.rta_val = 0;
  } else if (dst.ip.f.v6) {
    struct {
      struct rtmsg rtm;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[16];
      } rta_dst;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_oif;
    } * nl_req;

    struct nlmsghdr *nlh =
        nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_NEWROUTE, sizeof(*nl_req),
                  NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);

    nl_req = nlmsg_data(nlh);
    memset(nl_req, 0, sizeof(*nl_req));
    nl_req->rtm.rtm_family = FAMILY_V6;
    nl_req->rtm.rtm_dst_len = dst.mask;
    nl_req->rtm.rtm_table = RT_TABLE_MAIN;
    nl_req->rtm.rtm_scope = RT_SCOPE_UNIVERSE;
    nl_req->rta_dst.rta_type = RTA_DST;
    nl_req->rta_dst.rta_len = 20;
    memcpy(nl_req->rta_dst.rta_val, dst.ip.v6.bytes, 16);
    nl_req->rta_oif.rta_type = RTA_OIF;
    nl_req->rta_oif.rta_len = 8;
    nl_req->rta_oif.rta_val = 0;
  } else {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  int ret = nl_send_auto_complete(socket, msg);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  nlmsg_free(msg);
  nl_socket_free(socket);

  return true;
}

bool _internal_nl_addr_mod(const char *addr_str, const char *ifi_name,
                           int type) {
  if (strlen(addr_str) == 0 || strlen(ifi_name) == 0) {
    return false;
  }
  struct nl_label_net addr;
  memset(&addr, 0, sizeof(addr));
  if (!parse_label_net(addr_str, &addr) || (!addr.ip.f.v4 && !addr.ip.f.v6)) {
    return false;
  }
  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifi_name, &port);
  if (ret < 0 || port.index == 0) {
    return false;
  }
  if (strlen((char *)addr.label) > 0 &&
      strncmp((char *)addr.label, (char *)port.name,
              strlen((char *)port.name)) != 0) {
    return false;
  }

  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  struct {
    struct ifaddrmsg if_addr;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[4];
    } rta_local_v4;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[16];
    } rta_local_v6;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[4];
    } rta_addr_v4;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[16];
    } rta_addr_v6;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[4];
    } rta_broadcast;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[strlen((char *)addr.label) + 1];
    } rta_label;
  } * nl_req;

  struct nlmsghdr *nlh =
      nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, type, sizeof(*nl_req),
                NLM_F_REQUEST | NLM_F_EXCL | NLM_F_ACK);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->if_addr.ifa_index = port.index;
  nl_req->if_addr.ifa_scope = 0;
  nl_req->if_addr.ifa_prefixlen = addr.mask;

  nl_req->rta_local_v4.rta_type = IFA_UNSPEC;
  nl_req->rta_local_v4.rta_len = 8;
  nl_req->rta_local_v6.rta_type = IFA_UNSPEC;
  nl_req->rta_local_v6.rta_len = 20;
  nl_req->rta_addr_v4.rta_type = IFA_UNSPEC;
  nl_req->rta_addr_v4.rta_len = 8;
  nl_req->rta_addr_v6.rta_type = IFA_UNSPEC;
  nl_req->rta_addr_v6.rta_len = 20;
  nl_req->rta_broadcast.rta_type = IFA_UNSPEC;
  nl_req->rta_broadcast.rta_len = 8;
  nl_req->rta_label.rta_type = IFA_UNSPEC;
  nl_req->rta_label.rta_len = strlen((char *)addr.label) + 4 + 1;

  if (addr.ip.f.v4) {
    nl_req->if_addr.ifa_family = FAMILY_V4;
    nl_req->rta_local_v4.rta_type = IFA_LOCAL;
    nl_req->rta_local_v4.rta_len = 8;
    memcpy(nl_req->rta_local_v4.rta_val, addr.ip.v4.bytes, 4);

    nl_req->rta_addr_v4.rta_type = IFA_ADDRESS;
    nl_req->rta_addr_v4.rta_len = 8;
    memcpy(nl_req->rta_addr_v4.rta_val, addr.ip.v4.bytes, 4);

    if (addr.mask < 31) {
      nl_req->rta_broadcast.rta_type = IFA_BROADCAST;
      nl_req->rta_broadcast.rta_len = 8;
      __u8 mask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
      __u32 *mask_ip = (__u32 *)mask;
      *mask_ip = (*mask_ip) >> (32 - addr.mask);
      for (int i = 0; i < 4; i++) {
        mask[i] = ~mask[i];
        nl_req->rta_broadcast.rta_val[i] =
            nl_req->rta_local_v4.rta_val[i] | mask[i];
      }
    }
    if (strlen((char *)addr.label) > 0) {
      nl_req->rta_label.rta_type = IFA_LABEL;
      nl_req->rta_label.rta_len = strlen((char *)addr.label) + 4 + 1;
      memcpy(nl_req->rta_label.rta_val, (char *)addr.label,
             strlen((char *)addr.label));
      printf("nl_req->rta_label.rta_val==[%s] [%d]\n",
             (char *)nl_req->rta_label.rta_val, nl_req->rta_label.rta_len);
    }
  } else if (addr.ip.f.v6) {
    nl_req->if_addr.ifa_family = FAMILY_V6;
    nl_req->rta_local_v6.rta_type = IFA_LOCAL;
    nl_req->rta_local_v6.rta_len = 20;
    memcpy(nl_req->rta_local_v6.rta_val, addr.ip.v6.bytes, 16);
    nl_req->rta_addr_v6.rta_type = IFA_ADDRESS;
    nl_req->rta_addr_v6.rta_len = 20;
    memcpy(nl_req->rta_addr_v6.rta_val, addr.ip.v6.bytes, 16);
  }

  ret = nl_send_auto_complete(socket, msg);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  nlmsg_free(msg);
  nl_socket_free(socket);

  return true;
}

bool nl_addr_add(const char *addr_str, const char *ifi_name) {
  return _internal_nl_addr_mod(addr_str, ifi_name, RTM_NEWADDR);
}

bool nl_addr_del(const char *addr_str, const char *ifi_name) {
  return _internal_nl_addr_mod(addr_str, ifi_name, RTM_DELADDR);
}

bool _internal_nl_neigh_mod(nl_neigh_mod_t *neigh, int type, int flags) {
  __u8 family = neigh->family;
  if (family == 0) {
    if (neigh->ip.f.v4) {
      family = FAMILY_V4;
    } else {
      family = FAMILY_V6;
    }
  }

  __u8 ip_data_len = 0;
  if (neigh->ip.f.v4) {
    ip_data_len = 4;
  } else if (neigh->ip.f.v6) {
    ip_data_len = 16;
  }

  __u8 ll_addr_data_len = 0;
  if (neigh->ll_ip_addr.f.v4) {
    ll_addr_data_len = 4;
  } else if (neigh->flags != NTF_PROXY || !is_zero_mac(neigh->hwaddr)) {
    ll_addr_data_len = ETH_ALEN;
  }

  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  struct {
    struct ndmsg ndm;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[ip_data_len];
    } rta_dst;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[ll_addr_data_len];
    } rta_lladr;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u16 rta_val;
    } rta_vlan;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta_vni;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta_master;
  } * nl_req;

  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, type,
                                   sizeof(*nl_req), NLM_F_REQUEST | flags);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ndm.ndm_family = family;
  nl_req->ndm.ndm_ifindex = neigh->link_index;
  nl_req->ndm.ndm_state = neigh->state;
  nl_req->ndm.ndm_type = neigh->type;
  nl_req->ndm.ndm_flags = neigh->flags;

  nl_req->rta_dst.rta_type = NDA_DST;
  nl_req->rta_dst.rta_len = ip_data_len + 4;
  if (neigh->ip.f.v4) {
    memcpy(nl_req->rta_dst.rta_val, neigh->ip.v4.bytes, 4);
  } else if (neigh->ip.f.v6) {
    memcpy(nl_req->rta_dst.rta_val, neigh->ip.v6.bytes, 16);
  }

  nl_req->rta_lladr.rta_type = NDA_LLADDR;
  nl_req->rta_lladr.rta_len = ll_addr_data_len + 4;
  if (neigh->ll_ip_addr.f.v4) {
    memcpy(nl_req->rta_lladr.rta_val, neigh->ll_ip_addr.v4.bytes, 4);
  } else if (neigh->flags != NTF_PROXY || !is_zero_mac(neigh->hwaddr)) {
    memcpy(nl_req->rta_lladr.rta_val, neigh->hwaddr, ETH_ALEN);
  }

  nl_req->rta_vlan.rta_len = 6;
  if (neigh->vlan != 0) {
    nl_req->rta_vlan.rta_type = NDA_VLAN;
    nl_req->rta_vlan.rta_val = (__u16)neigh->vlan;
  } else {
    nl_req->rta_vlan.rta_type = IFA_UNSPEC;
  }

  nl_req->rta_vni.rta_len = 8;
  if (neigh->vni != 0) {
    nl_req->rta_vni.rta_type = NDA_VNI;
    nl_req->rta_vni.rta_val = neigh->vni;
  } else {
    nl_req->rta_vni.rta_type = IFA_UNSPEC;
  }

  nl_req->rta_master.rta_len = 8;
  if (neigh->master_index != 0) {
    nl_req->rta_master.rta_type = NDA_MASTER;
    nl_req->rta_master.rta_val = neigh->master_index;
  } else {
    nl_req->rta_master.rta_type = IFA_UNSPEC;
  }

  int ret = nl_send_auto_complete(socket, msg);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  nlmsg_free(msg);
  nl_socket_free(socket);

  return true;
}

bool _internal_nl_neigh_add(nl_neigh_mod_t *neigh) {
  return _internal_nl_neigh_mod(neigh, RTM_NEWNEIGH, NLM_F_ACK);
}

bool _internal_nl_neigh_append(nl_neigh_mod_t *neigh) {
  return _internal_nl_neigh_mod(neigh, RTM_NEWNEIGH,
                                NLM_F_CREATE | NLM_F_APPEND | NLM_F_ACK);
}

bool _internal_nl_neigh_del(nl_neigh_mod_t *neigh) {
  return _internal_nl_neigh_mod(neigh, RTM_DELNEIGH, NLM_F_ACK);
}

bool nl_fdb_add(const char *mac_addr, const char *ifi_name) {
  nl_neigh_mod_t neigh;
  memset(&neigh, 0, sizeof(neigh));

  if (!mac_pton(mac_addr, neigh.hwaddr)) {
    return false;
  }

  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifi_name, &port);
  if (ret < 0 || port.index == 0) {
    return false;
  }

  neigh.family = AF_BRIDGE;
  neigh.link_index = port.index;
  neigh.state = NUD_PERMANENT;
  neigh.flags = NTF_SELF;
  return _internal_nl_neigh_append(&neigh);
}

bool nl_fdb_del(const char *mac_addr, const char *ifi_name) {
  nl_neigh_mod_t neigh;
  memset(&neigh, 0, sizeof(neigh));
  if (!mac_pton(mac_addr, neigh.hwaddr)) {
    return false;
  }

  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifi_name, &port);
  if (ret < 0 || port.index == 0) {
    return false;
  }

  neigh.family = AF_BRIDGE;
  neigh.link_index = port.index;
  neigh.state = NUD_PERMANENT;
  neigh.flags = NTF_SELF;
  return _internal_nl_neigh_del(&neigh);
}

bool nl_neigh_add(const char *ip_addr, const char *ifi_name,
                  const char *mac_addr) {
  nl_neigh_mod_t neigh;
  memset(&neigh, 0, sizeof(neigh));
  if (!parse_ip(ip_addr, &neigh.ip)) {
    return false;
  }

  if (!mac_pton(mac_addr, neigh.hwaddr)) {
    return false;
  }

  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifi_name, &port);
  if (ret < 0 || port.index == 0) {
    return false;
  }

  neigh.link_index = port.index;
  neigh.state = NUD_PERMANENT;
  return _internal_nl_neigh_append(&neigh);
}

bool nl_neigh_del(const char *ip_addr, const char *ifi_name) {
  nl_neigh_mod_t neigh;
  memset(&neigh, 0, sizeof(neigh));
  if (!parse_ip(ip_addr, &neigh.ip)) {
    return false;
  }

  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifi_name, &port);
  if (ret < 0 || port.index == 0) {
    return false;
  }

  neigh.link_index = port.index;
  return _internal_nl_neigh_del(&neigh);
}

bool nl_vxlan_peer_add(__u32 vxlan_id, const char *peer_ip) {
  nl_neigh_mod_t peer;
  memset(&peer, 0, sizeof(peer));
  if (!parse_ip(peer_ip, &peer.ip)) {
    return false;
  }

  char *mac_addr = "00:00:00:00:00:00";
  if (!mac_pton(mac_addr, peer.hwaddr)) {
    return false;
  }

  char ifi_name[IF_NAMESIZE];
  snprintf(ifi_name, IF_NAMESIZE, "vxlan%d", vxlan_id);
  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifi_name, &port);
  if (ret < 0 || port.index == 0) {
    return false;
  }

  peer.family = AF_BRIDGE;
  peer.link_index = port.index;
  peer.state = NUD_PERMANENT;
  peer.flags = NTF_SELF;
  return _internal_nl_neigh_append(&peer);
}

bool nl_vxlan_peer_del(__u32 vxlan_id, const char *peer_ip) {
  nl_neigh_mod_t peer;
  memset(&peer, 0, sizeof(peer));
  if (!parse_ip(peer_ip, &peer.ip)) {
    return false;
  }

  char *mac_addr = "00:00:00:00:00:00";
  if (!mac_pton(mac_addr, peer.hwaddr)) {
    return false;
  }

  char ifi_name[IF_NAMESIZE];
  snprintf(ifi_name, IF_NAMESIZE, "vxlan%d", vxlan_id);
  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifi_name, &port);
  if (ret < 0 || port.index == 0) {
    return false;
  }

  peer.family = AF_BRIDGE;
  peer.link_index = port.index;
  peer.state = NUD_PERMANENT;
  peer.flags = NTF_SELF;
  return _internal_nl_neigh_del(&peer);
}

bool _internal_nl_link_mod(nl_link_t *link, int flags) {
  nl_base_attrs_t base = link->attrs;
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();
  int ret = 0;

  struct ifinfomsg *nl_req;
  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_NEWLINK,
                                   sizeof(*nl_req), NLM_F_REQUEST | flags);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ifi_family = AF_UNSPEC;

  if ((base.flags & FLAG_UP) != 0) {
    nl_req->ifi_change = IFF_UP;
    nl_req->ifi_flags = IFF_UP;
  }

  if ((base.flags & FLAG_BROADCAST) != 0) {
    nl_req->ifi_change = IFF_BROADCAST;
    nl_req->ifi_flags = IFF_BROADCAST;
  }

  if ((base.flags & FLAG_LOOPBACK) != 0) {
    nl_req->ifi_change = IFF_LOOPBACK;
    nl_req->ifi_flags = IFF_LOOPBACK;
  }

  if ((base.flags & FLAG_POINTTOPOINT) != 0) {
    nl_req->ifi_change = IFF_POINTOPOINT;
    nl_req->ifi_flags = IFF_POINTOPOINT;
  }

  if ((base.flags & FLAG_MULTICAST) != 0) {
    nl_req->ifi_change = IFF_MULTICAST;
    nl_req->ifi_flags = IFF_MULTICAST;
  }

  if (base.index != 0) {
    nl_req->ifi_index = base.index;
  }

  if (base.parent_index != 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_LINK;
    rta.rta_len = sizeof(rta);
    rta.rta_val = (__u32)base.parent_index;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.ipvlan) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    printf("can't create ipvlan link without ParentIndex");
    return false;
  } else if (link->type.ipoib) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    printf("can't create ipoib link without ParentIndex");
    return false;
  }

  struct {
    __u16 rta_len;
    __u16 rta_type;
    __u8 rta_val[RTA_ALIGN(strlen(base.name))];
  } rta_name;
  memset(&rta_name, 0, sizeof(rta_name));
  rta_name.rta_type = IFLA_IFNAME;
  rta_name.rta_len = sizeof(rta_name);
  memcpy(&rta_name.rta_val, base.name, strlen(base.name));
  ret = nlmsg_append(msg, &rta_name, sizeof(rta_name), RTA_PADDING(rta_name));
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  if (base.mtu > 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_MTU;
    rta.rta_len = sizeof(rta);
    rta.rta_val = (__u32)base.mtu;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.tx_q_len >= 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_TXQLEN;
    rta.rta_len = sizeof(rta);
    rta.rta_val = (__u32)base.tx_q_len;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (!is_zero_mac(base.hw_addr)) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[ETH_ALEN];
      __u8 padding[2];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_ADDRESS;
    rta.rta_len = sizeof(rta);
    memcpy(rta.rta_val, base.hw_addr, ETH_ALEN);
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.num_tx_queues > 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_NUM_TX_QUEUES;
    rta.rta_len = sizeof(rta);
    rta.rta_val = (__u32)base.num_tx_queues;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.num_rx_queues > 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_NUM_RX_QUEUES;
    rta.rta_len = sizeof(rta);
    rta.rta_val = (__u32)base.num_rx_queues;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.gso_max_segs > 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_GSO_MAX_SEGS;
    rta.rta_len = sizeof(rta);
    rta.rta_val = base.gso_max_segs;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.gso_max_size > 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_GSO_MAX_SIZE;
    rta.rta_len = sizeof(rta);
    rta.rta_val = (__u32)base.gso_max_size;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.group > 0) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_GROUP;
    rta.rta_len = sizeof(rta);
    rta.rta_val = base.group;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.namespace != NULL &&
      (base.namespace->type.ns_pid || base.namespace->type.ns_fd)) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));

    if (base.namespace->type.ns_pid) {
      rta.rta_type = IFLA_NET_NS_PID;
    } else if (base.namespace->type.ns_fd) {
      rta.rta_type = IFLA_NET_NS_FD;
    }

    rta.rta_len = sizeof(rta);
    rta.rta_val = base.namespace->ns;
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (base.xdp) {
    int nested_attrs = 1;
    if (base.xdp->flags != 0) {
      nested_attrs = 2;
    }
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_vals[nested_attrs];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_XDP | NLA_F_NESTED;
    rta.rta_len = sizeof(rta);
    rta.rta_vals[0].rta_type = IFLA_XDP_FD;
    rta.rta_vals[0].rta_val = (__u32)base.xdp->fd;
    rta.rta_vals[0].rta_len = sizeof(rta.rta_vals[0]);
    if (base.xdp->flags != 0) {
      rta.rta_vals[1].rta_type = IFLA_XDP_FLAGS;
      rta.rta_vals[1].rta_val = base.xdp->flags;
      rta.rta_vals[1].rta_len = sizeof(rta.rta_vals[1]);
    }
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  char *type = link_type(&link->type);
  struct {
    __u16 rta_len;
    __u16 rta_type;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[RTA_ALIGN(strlen(type))];
    } rta_val;
  } rta_info;
  memset(&rta_info, 0, sizeof(rta_info));
  rta_info.rta_type = IFLA_LINKINFO;
  rta_info.rta_len = sizeof(rta_info);
  rta_info.rta_val.rta_type = IFLA_INFO_KIND;
  rta_info.rta_val.rta_len = sizeof(rta_info.rta_val);
  memcpy(&rta_info.rta_val.rta_val, type, strlen(type));
  ret = nlmsg_append(msg, &rta_info, sizeof(rta_info), RTA_PADDING(rta_info));
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  if (link->type.vlan) {
    int nested_attrs = 1;
    if (link->u.vlan.vlan_protocol != 0) {
      nested_attrs = 2;
    }
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_vals[nested_attrs];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
    rta.rta_vals[0].rta_type = IFLA_VLAN_ID;
    rta.rta_vals[0].rta_val = (__u16)link->u.vlan.vlan_id;
    rta.rta_vals[0].rta_len = sizeof(rta.rta_vals[0]);
    if (link->u.vlan.vlan_protocol != 0) {
      rta.rta_vals[1].rta_type = IFLA_VLAN_PROTOCOL;
      rta.rta_vals[1].rta_val = htons(link->u.vlan.vlan_protocol);
      rta.rta_vals[1].rta_len = sizeof(rta.rta_vals[1]);
    }
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.veth) {
    int nested_u8s = 0, nested_u8s_idx = -1;
    int nested_u32 = 0, nested_u32_idx = -1;
    if (base.tx_q_len >= 0) {
      nested_u32++;
    }
    if (base.mtu > 0) {
      nested_u32++;
    }
    if (!is_zero_mac(link->u.veth.peer_hw_addr)) {
      nested_u8s++;
    }
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        struct {
          __u16 rta_len;
          __u16 rta_type;
          __u8 rta_val[IF_NAMESIZE];
        } rta_ifi_name;
        struct {
          __u16 rta_len;
          __u16 rta_type;
          __u32 rta_val;
        } rta_u32_vals[nested_u32];
        struct {
          __u16 rta_len;
          __u16 rta_type;
          __u8 rta_val[ETH_ALEN];
        } rta_u8s_vals[nested_u8s];
      } rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
#define VETH_INFO_PEER 1
    rta.rta_val.rta_type = VETH_INFO_PEER;
    rta.rta_val.rta_len = sizeof(rta.rta_val);
    rta.rta_val.rta_ifi_name.rta_type = IFLA_IFNAME;
    memcpy(rta.rta_val.rta_ifi_name.rta_val, link->u.veth.peer_name,
           strlen(link->u.veth.peer_name));
    rta.rta_val.rta_ifi_name.rta_len = sizeof(rta.rta_val.rta_ifi_name);
    if (base.tx_q_len >= 0) {
      nested_u32_idx++;
      rta.rta_val.rta_u32_vals[nested_u32_idx].rta_type = IFLA_TXQLEN;
      rta.rta_val.rta_u32_vals[nested_u32_idx].rta_val = (__u32)base.tx_q_len;
      rta.rta_val.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_val.rta_u32_vals[nested_u32_idx]);
    }
    if (base.mtu > 0) {
      nested_u32_idx++;
      rta.rta_val.rta_u32_vals[nested_u32_idx].rta_type = IFLA_MTU;
      rta.rta_val.rta_u32_vals[nested_u32_idx].rta_val = (__u32)base.mtu;
      rta.rta_val.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_val.rta_u32_vals[nested_u32_idx]);
    }
    if (!is_zero_mac(link->u.veth.peer_hw_addr)) {
      nested_u8s_idx++;
      rta.rta_val.rta_u8s_vals[nested_u8s_idx].rta_type = IFLA_ADDRESS;
      memcpy(rta.rta_val.rta_u8s_vals[nested_u8s_idx].rta_val,
             link->u.veth.peer_hw_addr, ETH_ALEN);
      rta.rta_val.rta_u8s_vals[nested_u8s_idx].rta_len =
          sizeof(rta.rta_val.rta_u8s_vals[nested_u8s_idx]);
    }
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.vxlan) {
    int nested_u8 = 9, nested_u8_idx = -1;
    int nested_u16 = 0, nested_u16_idx = -1;
    int nested_u32 = 1, nested_u32_idx = -1;
    int nested_ipv4 = 0, nested_ipv4_idx = -1;
    int nested_ipv6 = 0, nested_ipv6_idx = -1;
    int nested_ports = 0, nested_ports_idx = -1;
    int nested_gbp = 0, nested_gbp_idx = -1;

    if (link->u.vxlan.vtep_dev_index != 0) {
      nested_u32++;
    }

    if (link->u.vxlan.src_addr) {
      if (link->u.vxlan.src_addr->f.v4) {
        nested_ipv4++;
      } else if (link->u.vxlan.src_addr->f.v6) {
        nested_ipv6++;
      }
    }

    if (link->u.vxlan.group) {
      if (link->u.vxlan.group->f.v4) {
        nested_ipv4++;
      } else if (link->u.vxlan.group->f.v6) {
        nested_ipv6++;
      }
    }

    if (link->u.vxlan.udp_csum) {
      nested_u8++;
    }

    if (link->u.vxlan.flow_based) {
      nested_u8++;
    }

    if (link->u.vxlan.no_age) {
      nested_u32++;
    } else if (link->u.vxlan.age > 0) {
      nested_u32++;
    }

    if (link->u.vxlan.limit > 0) {
      nested_u32++;
    }

    if (link->u.vxlan.port > 0) {
      nested_u16++;
    }

    if (link->u.vxlan.port_low > 0 || link->u.vxlan.port_high > 0) {
      nested_ports++;
    }

    if (link->u.vxlan.gbp) {
      nested_gbp++;
    }

    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val;
      } rta_u8_vals[nested_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_u16_vals[nested_u16];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_u32_vals[nested_u32];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_ipv4_vals[nested_ipv4];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[16];
      } rta_ipv6_vals[nested_ipv6];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 lo;
        __u16 hi;
      } rta_ports_vals[nested_ports];
      struct {
        __u16 rta_len;
        __u16 rta_type;
      } rta_gbp_vals[nested_gbp];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);

    nested_u32_idx++;
    rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_VXLAN_ID;
    if (link->u.vxlan.flow_based) {
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)0;
    } else {
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.vxlan.vxlan_id;
    }

    rta.rta_u32_vals[nested_u32_idx].rta_len =
        sizeof(rta.rta_u32_vals[nested_u32_idx]);

    if (link->u.vxlan.vtep_dev_index != 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_VXLAN_LINK;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.vxlan.vtep_dev_index;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }

    if (link->u.vxlan.src_addr) {
      if (link->u.vxlan.src_addr->f.v4) {
        nested_ipv4_idx++;
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_VXLAN_LOCAL;
        memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
               link->u.vxlan.src_addr->v4.bytes, 4);
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
            sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
      } else if (link->u.vxlan.src_addr->f.v6) {
        nested_ipv6_idx++;
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_type = IFLA_VXLAN_LOCAL6;
        memcpy(rta.rta_ipv6_vals[nested_ipv6_idx].rta_val,
               link->u.vxlan.src_addr->v6.bytes, 16);
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_len =
            sizeof(rta.rta_ipv6_vals[nested_ipv6_idx]);
      }
    }

    if (link->u.vxlan.group) {
      if (link->u.vxlan.group->f.v4) {
        nested_ipv4_idx++;
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_VXLAN_GROUP;
        memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
               link->u.vxlan.group->v4.bytes, 4);
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
            sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
      } else if (link->u.vxlan.group->f.v6) {
        nested_ipv6_idx++;
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_type = IFLA_VXLAN_GROUP6;
        memcpy(rta.rta_ipv6_vals[nested_ipv6_idx].rta_val,
               link->u.vxlan.group->v6.bytes, 16);
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_len =
            sizeof(rta.rta_ipv6_vals[nested_ipv6_idx]);
      }
    }

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_TTL;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.ttl;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_TOS;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.tos;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_LEARNING;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.learning;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_PROXY;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.proxy;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_RSC;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.rsc;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_L2MISS;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.l2_miss;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_L3MISS;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.l3_miss;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_UDP_ZERO_CSUM6_TX;
    rta.rta_u8_vals[nested_u8_idx].rta_val =
        (__u8)link->u.vxlan.udp6_zero_csum_tx;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_UDP_ZERO_CSUM6_RX;
    rta.rta_u8_vals[nested_u8_idx].rta_val =
        (__u8)link->u.vxlan.udp6_zero_csum_rx;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    if (link->u.vxlan.udp_csum) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_UDP_CSUM;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.udp_csum;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }

    if (link->u.vxlan.flow_based) {
      nested_u8_idx++;
#define IFLA_VXLAN_FLOWBASED 25
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_VXLAN_FLOWBASED;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.vxlan.flow_based;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }

    if (link->u.vxlan.no_age) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_VXLAN_AGEING;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)0;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    } else if (link->u.vxlan.age > 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_VXLAN_AGEING;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.vxlan.age;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }

    if (link->u.vxlan.limit > 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_VXLAN_LIMIT;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.vxlan.limit;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }

    if (link->u.vxlan.port > 0) {
      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_VXLAN_PORT;
      rta.rta_u16_vals[nested_u16_idx].rta_val = htons(link->u.vxlan.port);
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);
    }

    if (link->u.vxlan.port_low > 0 || link->u.vxlan.port_high > 0) {
      nested_ports_idx++;
      rta.rta_ports_vals[nested_ports_idx].rta_type = IFLA_VXLAN_PORT_RANGE;
      rta.rta_ports_vals[nested_ports_idx].lo = htons(link->u.vxlan.port_low);
      rta.rta_ports_vals[nested_ports_idx].hi = htons(link->u.vxlan.port_high);
      rta.rta_ports_vals[nested_ports_idx].rta_len =
          sizeof(rta.rta_ports_vals[nested_ports_idx]);
    }

    if (link->u.vxlan.gbp) {
      nested_gbp_idx++;
      rta.rta_gbp_vals[nested_gbp_idx].rta_type = IFLA_VXLAN_GBP;
      rta.rta_gbp_vals[nested_gbp_idx].rta_len =
          sizeof(rta.rta_gbp_vals[nested_gbp_idx]);
    }

    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.bond) {
    int nested_u8 = 9, nested_u8_idx = -1;
    int nested_u8s = 9, nested_u8s_idx = -1;
    int nested_u16 = 1, nested_u16_idx = -1;
    int nested_u32 = 1, nested_u32_idx = -1;
    int nested_ipv4 = 0, nested_ipv4_idx = -1;
    int nested_ipv6 = 0, nested_ipv6_idx = -1;

    if (link->u.bond.mode >= 0) {
      nested_u8++;
    }
    if (link->u.bond.activeslave >= 0) {
      nested_u32++;
    }
    if (link->u.bond.miimon >= 0) {
      nested_u32++;
    }
    if (link->u.bond.up_delay >= 0) {
      nested_u32++;
    }
    if (link->u.bond.down_delay >= 0) {
      nested_u32++;
    }
    if (link->u.bond.use_carrier >= 0) {
      nested_u8++;
    }
    if (link->u.bond.arp_interval >= 0) {
      nested_u32++;
    }
    if (link->u.bond.arp_validate >= 0) {
      nested_u32++;
    }
    if (link->u.bond.arp_all_targets >= 0) {
      nested_u32++;
    }
    if (link->u.bond.primary >= 0) {
      nested_u32++;
    }
    if (link->u.bond.primary_reselect >= 0) {
      nested_u8++;
    }
    if (link->u.bond.failover_mac >= 0) {
      nested_u8++;
    }
    if (link->u.bond.xmit_hash_policy >= 0) {
      nested_u8++;
    }
    if (link->u.bond.resend_igmp >= 0) {
      nested_u32++;
    }
    if (link->u.bond.num_peer_notif >= 0) {
      nested_u8++;
    }
    if (link->u.bond.all_slaves_active >= 0) {
      nested_u8++;
    }
    if (link->u.bond.min_links >= 0) {
      nested_u32++;
    }
    if (link->u.bond.lp_interval >= 0) {
      nested_u32++;
    }
    if (link->u.bond.packers_per_slave >= 0) {
      nested_u32++;
    }
    if (link->u.bond.lacp_rate >= 0) {
      nested_u8++;
    }
    if (link->u.bond.ad_select >= 0) {
      nested_u8++;
    }
    if (link->u.bond.ad_actor_sys_prio >= 0) {
      nested_u16++;
    }
    if (link->u.bond.ad_user_port_key >= 0) {
      nested_u16++;
    }
    if (link->u.bond.tlb_dynamic_lb >= 0) {
      nested_u8++;
    }
    if (!is_zero_mac(link->u.bond.ad_actor_system)) {
      nested_u8s++;
    }
    if (link->u.bond.arp_ip_targets) {
      int num = sizeof(*link->u.bond.arp_ip_targets) / sizeof(nl_ip_t);
      for (int i = 0; i < num; i++) {
        if (link->u.bond.arp_ip_targets[i].f.v4) {
          nested_ipv4++;
        } else if (link->u.bond.arp_ip_targets[i].f.v6) {
          nested_ipv6++;
        }
      }
    }

    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val;
      } rta_u8_vals[nested_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_u16_vals[nested_u16];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_u32_vals[nested_u32];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[ETH_ALEN];
      } rta_u8s_vals[nested_u8s];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_ipv4_vals[nested_ipv4];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[16];
      } rta_ipv6_vals[nested_ipv6];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);

    if (link->u.bond.mode >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_MODE;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.bond.mode;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.activeslave >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_ACTIVE_SLAVE;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.bond.activeslave;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.miimon >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_MIIMON;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.bond.miimon;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.up_delay >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_UPDELAY;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.bond.up_delay;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.down_delay >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_DOWNDELAY;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.bond.down_delay;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.use_carrier >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_USE_CARRIER;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.bond.use_carrier;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.arp_interval >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_ARP_INTERVAL;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.bond.arp_interval;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.arp_validate >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_ARP_VALIDATE;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.bond.arp_validate;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.arp_all_targets >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_ARP_ALL_TARGETS;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.bond.arp_all_targets;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.primary >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_PRIMARY;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.bond.primary;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.primary_reselect >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_PRIMARY_RESELECT;
      rta.rta_u8_vals[nested_u8_idx].rta_val =
          (__u8)link->u.bond.primary_reselect;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.failover_mac >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_FAIL_OVER_MAC;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.bond.failover_mac;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.xmit_hash_policy >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_XMIT_HASH_POLICY;
      rta.rta_u8_vals[nested_u8_idx].rta_val =
          (__u8)link->u.bond.xmit_hash_policy;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.resend_igmp >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_RESEND_IGMP;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.bond.resend_igmp;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.num_peer_notif >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_NUM_PEER_NOTIF;
      rta.rta_u8_vals[nested_u8_idx].rta_val =
          (__u8)link->u.bond.num_peer_notif;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.all_slaves_active >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_ALL_SLAVES_ACTIVE;
      rta.rta_u8_vals[nested_u8_idx].rta_val =
          (__u8)link->u.bond.all_slaves_active;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.min_links >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_MIN_LINKS;
      rta.rta_u32_vals[nested_u32_idx].rta_val = (__u32)link->u.bond.min_links;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.lp_interval >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_LP_INTERVAL;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.bond.lp_interval;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.packers_per_slave >= 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BOND_PACKETS_PER_SLAVE;
      rta.rta_u32_vals[nested_u32_idx].rta_val =
          (__u32)link->u.bond.packers_per_slave;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bond.lacp_rate >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_AD_LACP_RATE;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.bond.lacp_rate;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.ad_select >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_AD_SELECT;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.bond.ad_select;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bond.ad_actor_sys_prio >= 0) {
      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_BOND_AD_ACTOR_SYS_PRIO;
      rta.rta_u16_vals[nested_u16_idx].rta_val =
          (__u16)link->u.bond.ad_actor_sys_prio;
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);
    }
    if (link->u.bond.ad_user_port_key >= 0) {
      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_BOND_AD_USER_PORT_KEY;
      rta.rta_u16_vals[nested_u16_idx].rta_val =
          (__u16)link->u.bond.ad_user_port_key;
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);
    }
    if (link->u.bond.tlb_dynamic_lb >= 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BOND_TLB_DYNAMIC_LB;
      rta.rta_u8_vals[nested_u8_idx].rta_val =
          (__u8)link->u.bond.tlb_dynamic_lb;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (!is_zero_mac(link->u.bond.ad_actor_system)) {
      nested_u8s_idx++;
      rta.rta_u8s_vals[nested_u8s_idx].rta_type = IFLA_BOND_AD_ACTOR_SYSTEM;
      memcpy(rta.rta_u8s_vals[nested_u8s_idx].rta_val,
             link->u.bond.ad_actor_system, ETH_ALEN);
      rta.rta_u8s_vals[nested_u8s_idx].rta_len =
          sizeof(rta.rta_u8s_vals[nested_u8s_idx]);
    }

    if (link->u.bond.arp_ip_targets) {
      int num = sizeof(*link->u.bond.arp_ip_targets) / sizeof(nl_ip_t);
      for (int i = 0; i < num; i++) {
        if (link->u.bond.arp_ip_targets[i].f.v4) {
          nested_ipv4_idx++;
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_BOND_ARP_IP_TARGET;
          memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
                 link->u.bond.arp_ip_targets[i].v4.bytes, 4);
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
              sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
        } else if (link->u.bond.arp_ip_targets[i].f.v6) {
          nested_ipv6_idx++;
          rta.rta_ipv6_vals[nested_ipv6_idx].rta_type = IFLA_BOND_ARP_IP_TARGET;
          memcpy(rta.rta_ipv6_vals[nested_ipv6_idx].rta_val,
                 link->u.bond.arp_ip_targets[i].v6.bytes, 16);
          rta.rta_ipv6_vals[nested_ipv6_idx].rta_len =
              sizeof(rta.rta_ipv6_vals[nested_ipv6_idx]);
        }
      }
    }

    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.ipvlan) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_mode;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_flag;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
    rta.rta_mode.rta_type = IFLA_IPVLAN_MODE;
    rta.rta_mode.rta_val = link->u.ipvlan.mode;
    rta.rta_mode.rta_len = sizeof(rta.rta_mode.rta_val);
    rta.rta_flag.rta_type = IFLA_IPVLAN_FLAGS;
    rta.rta_flag.rta_val = link->u.ipvlan.flag;
    rta.rta_flag.rta_len = sizeof(rta.rta_flag.rta_val);
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.macvlan) {
    if (link->u.macvlan.mode != 0) {
      struct {
        __u16 rta_len;
        __u16 rta_type;
        struct {
          __u16 rta_len;
          __u16 rta_type;
          __u32 rta_val;
        } rta_val;
      } rta;
      memset(&rta, 0, sizeof(rta));
      rta.rta_type = IFLA_INFO_DATA;
      rta.rta_len = sizeof(rta);
      rta.rta_val.rta_type = IFLA_MACVLAN_MODE;
      rta.rta_val.rta_val = (__u32)link->u.macvlan.mode;
      rta.rta_val.rta_len = sizeof(rta.rta_val.rta_val);
      ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
      if (ret < 0) {
        nlmsg_free(msg);
        nl_socket_free(socket);
        return false;
      }
    }
  } else if (link->type.macvtap) {
    if (link->u.macvtap.mode != 0) {
      struct {
        __u16 rta_len;
        __u16 rta_type;
        struct {
          __u16 rta_len;
          __u16 rta_type;
          __u32 rta_val;
        } rta_val;
      } rta;
      memset(&rta, 0, sizeof(rta));
      rta.rta_type = IFLA_INFO_DATA;
      rta.rta_len = sizeof(rta);
      rta.rta_val.rta_type = IFLA_MACVLAN_MODE;
      rta.rta_val.rta_val = (__u32)link->u.macvtap.mode;
      rta.rta_val.rta_len = sizeof(rta.rta_val.rta_val);
      ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
      if (ret < 0) {
        nlmsg_free(msg);
        nl_socket_free(socket);
        return false;
      }
    }
  } else if (link->type.gretap) {
    int nested_u8 = 0, nested_u8_idx = -1;
    int nested_u16 = 0, nested_u16_idx = -1;
    int nested_u32 = 0, nested_u32_idx = -1;
    int nested_ipv4 = 0, nested_ipv4_idx = -1;

    if (link->u.gretap.flow_based) {
      nested_u8 = 1;
    } else {
      nested_u8 = 3;
      nested_u16 = 6;
      if (link->u.gretap.local) {
        if (link->u.gretap.local->f.v4) {
          nested_ipv4++;
        }
      }
      if (link->u.gretap.remote) {
        if (link->u.gretap.remote->f.v4) {
          nested_ipv4++;
        }
      }
      if (link->u.gretap.ikey != 0) {
        nested_u32++;
      }
      if (link->u.gretap.okey != 0) {
        nested_u32++;
      }
      if (link->u.gretap.link != 0) {
        nested_u32++;
      }
    }

    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val;
      } rta_u8_vals[nested_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_u16_vals[nested_u16];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_u32_vals[nested_u32];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_ipv4_vals[nested_ipv4];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);

    if (link->u.gretap.flow_based) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_GRE_COLLECT_METADATA;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.gretap.flow_based;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    } else {
      if (link->u.gretap.local) {
        if (link->u.gretap.local->f.v4) {
          nested_ipv4_idx++;
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_GRE_LOCAL;
          memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
                 link->u.gretap.local->v4.bytes, 4);
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
              sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
        }
      }
      if (link->u.gretap.remote) {
        if (link->u.gretap.remote->f.v4) {
          nested_ipv4_idx++;
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_GRE_REMOTE;
          memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
                 link->u.gretap.remote->v4.bytes, 4);
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
              sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
        }
      }
      if (link->u.gretap.ikey != 0) {
        nested_u32_idx++;
        rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_GRE_COLLECT_METADATA;
        rta.rta_u32_vals[nested_u32_idx].rta_val = htonl(link->u.gretap.ikey);
        rta.rta_u32_vals[nested_u32_idx].rta_len =
            sizeof(rta.rta_u32_vals[nested_u32_idx]);
        link->u.gretap.iflags |= GRE_KEY;
      }
      if (link->u.gretap.okey != 0) {
        nested_u32_idx++;
        rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_GRE_COLLECT_METADATA;
        rta.rta_u32_vals[nested_u32_idx].rta_val = htonl(link->u.gretap.okey);
        rta.rta_u32_vals[nested_u32_idx].rta_len =
            sizeof(rta.rta_u32_vals[nested_u32_idx]);
        link->u.gretap.oflags |= GRE_KEY;
      }

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_GRE_IFLAGS;
      rta.rta_u16_vals[nested_u16_idx].rta_val = htons(link->u.gretap.iflags);
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_GRE_OFLAGS;
      rta.rta_u16_vals[nested_u16_idx].rta_val = htons(link->u.gretap.oflags);
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      if (link->u.gretap.link != 0) {
        nested_u32_idx++;
        rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_GRE_LINK;
        rta.rta_u32_vals[nested_u32_idx].rta_val = link->u.gretap.link;
        rta.rta_u32_vals[nested_u32_idx].rta_len =
            sizeof(rta.rta_u32_vals[nested_u32_idx]);
      }

      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_GRE_PMTUDISC;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.gretap.p_mtu_disc;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);

      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_GRE_TTL;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.gretap.ttl;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);

      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_GRE_TOS;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.gretap.tos;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_GRE_ENCAP_TYPE;
      rta.rta_u16_vals[nested_u16_idx].rta_val = link->u.gretap.encap_type;
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_GRE_ENCAP_FLAGS;
      rta.rta_u16_vals[nested_u16_idx].rta_val = link->u.gretap.encap_flags;
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_GRE_ENCAP_SPORT;
      rta.rta_u16_vals[nested_u16_idx].rta_val =
          htons(link->u.gretap.encap_s_port);
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_GRE_ENCAP_DPORT;
      rta.rta_u16_vals[nested_u16_idx].rta_val =
          htons(link->u.gretap.encap_d_port);
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);
    }

    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.ipip) {
    int direct_u8 = 0, direct_u8_idx = -1;
    int nested_u8 = 0, nested_u8_idx = -1;
    int nested_u16 = 0, nested_u16_idx = -1;
    int nested_u32 = 0, nested_u32_idx = -1;
    int nested_ipv4 = 0, nested_ipv4_idx = -1;

    if (link->u.iptun.flow_based) {
      direct_u8 = 1;
    } else {
      nested_u8 = 3;
      nested_u16 = 4;
      if (link->u.iptun.local) {
        if (link->u.iptun.local->f.v4) {
          nested_ipv4++;
        }
      }
      if (link->u.iptun.remote) {
        if (link->u.iptun.remote->f.v4) {
          nested_ipv4++;
        }
      }
      if (link->u.iptun.link != 0) {
        nested_u32++;
      }
    }

    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u8 rta_val[direct_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val;
      } rta_u8_vals[nested_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_u16_vals[nested_u16];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_u32_vals[nested_u32];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_ipv4_vals[nested_ipv4];
    } rta;
    memset(&rta, 0, sizeof(rta));

    if (link->u.iptun.flow_based) {
      direct_u8_idx++;
      rta.rta_type = IFLA_IPTUN_COLLECT_METADATA;
      rta.rta_len = sizeof(rta);
      rta.rta_val[direct_u8_idx] = (__u8)link->u.iptun.flow_based;
    } else {
      rta.rta_type = IFLA_INFO_DATA;
      rta.rta_len = sizeof(rta);
      if (link->u.iptun.local) {
        if (link->u.iptun.local->f.v4) {
          nested_ipv4_idx++;
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_IPTUN_LOCAL;
          memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
                 link->u.iptun.local->v4.bytes, 4);
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
              sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
        }
      }
      if (link->u.iptun.remote) {
        if (link->u.iptun.remote->f.v4) {
          nested_ipv4_idx++;
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_IPTUN_REMOTE;
          memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
                 link->u.iptun.remote->v4.bytes, 4);
          rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
              sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
        }
      }

      if (link->u.iptun.link != 0) {
        nested_u32_idx++;
        rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_IPTUN_LINK;
        rta.rta_u32_vals[nested_u32_idx].rta_val = link->u.iptun.link;
        rta.rta_u32_vals[nested_u32_idx].rta_len =
            sizeof(rta.rta_u32_vals[nested_u32_idx]);
      }

      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_PMTUDISC;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.iptun.p_mtu_disc;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);

      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_TTL;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.iptun.ttl;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);

      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_TOS;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.iptun.tos;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_TYPE;
      rta.rta_u16_vals[nested_u16_idx].rta_val = link->u.iptun.encap_type;
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_FLAGS;
      rta.rta_u16_vals[nested_u16_idx].rta_val = link->u.iptun.encap_flags;
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_SPORT;
      rta.rta_u16_vals[nested_u16_idx].rta_val =
          htons(link->u.iptun.encap_s_port);
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);

      nested_u16_idx++;
      rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_DPORT;
      rta.rta_u16_vals[nested_u16_idx].rta_val =
          htons(link->u.iptun.encap_d_port);
      rta.rta_u16_vals[nested_u16_idx].rta_len =
          sizeof(rta.rta_u16_vals[nested_u16_idx]);
    }

    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.ip6tnl) {
    int nested_u8 = 4, nested_u8_idx = -1;
    int nested_u32 = 2, nested_u32_idx = -1;
    int nested_ipv6 = 0, nested_ipv6_idx = -1;

    if (link->u.ip6tnl.link != 0) {
      nested_u32++;
    }
    if (link->u.ip6tnl.local) {
      if (link->u.ip6tnl.local->f.v6) {
        nested_ipv6++;
      }
    }
    if (link->u.ip6tnl.remote) {
      if (link->u.ip6tnl.remote->f.v6) {
        nested_ipv6++;
      }
    }

    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val;
      } rta_u8_vals[nested_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_u32_vals[nested_u32];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[16];
      } rta_ipv6_vals[nested_ipv6];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);

    if (link->u.ip6tnl.link != 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_IPTUN_LINK;
      rta.rta_u32_vals[nested_u32_idx].rta_val = link->u.ip6tnl.link;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }

    if (link->u.ip6tnl.local) {
      if (link->u.ip6tnl.local->f.v6) {
        nested_ipv6_idx++;
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_type = IFLA_IPTUN_LOCAL;
        memcpy(rta.rta_ipv6_vals[nested_ipv6_idx].rta_val,
               link->u.ip6tnl.local->v6.bytes, 16);
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_len =
            sizeof(rta.rta_ipv6_vals[nested_ipv6_idx]);
      }
    }
    if (link->u.ip6tnl.remote) {
      if (link->u.ip6tnl.remote->f.v6) {
        nested_ipv6_idx++;
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_type = IFLA_IPTUN_REMOTE;
        memcpy(rta.rta_ipv6_vals[nested_ipv6_idx].rta_val,
               link->u.ip6tnl.remote->v6.bytes, 16);
        rta.rta_ipv6_vals[nested_ipv6_idx].rta_len =
            sizeof(rta.rta_ipv6_vals[nested_ipv6_idx]);
      }
    }

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_TTL;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.ip6tnl.ttl;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_TOS;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.ip6tnl.tos;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_ENCAP_LIMIT;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.ip6tnl.encap_limit;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u32_idx++;
    rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_IPTUN_FLAGS;
    rta.rta_u32_vals[nested_u32_idx].rta_val = link->u.ip6tnl.flags;
    rta.rta_u32_vals[nested_u32_idx].rta_len =
        sizeof(rta.rta_u32_vals[nested_u32_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_PROTO;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.ip6tnl.proto;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u32_idx++;
    rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_IPTUN_FLOWINFO;
    rta.rta_u32_vals[nested_u32_idx].rta_val = link->u.ip6tnl.flow_info;
    rta.rta_u32_vals[nested_u32_idx].rta_len =
        sizeof(rta.rta_u32_vals[nested_u32_idx]);

    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.sit) {
    int nested_u8 = 2, nested_u8_idx = -1;
    int nested_u16 = 4, nested_u16_idx = -1;
    int nested_u32 = 0, nested_u32_idx = -1;
    int nested_ipv4 = 0, nested_ipv4_idx = -1;

    if (link->u.sittun.link != 0) {
      nested_u32++;
    }
    if (link->u.sittun.local) {
      if (link->u.gretap.local->f.v4) {
        nested_ipv4++;
      }
    }
    if (link->u.sittun.remote) {
      if (link->u.gretap.remote->f.v4) {
        nested_ipv4++;
      }
    }
    if (link->u.sittun.ttl > 0) {
      nested_u8++;
    }

    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val;
      } rta_u8_vals[nested_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_u16_vals[nested_u16];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_u32_vals[nested_u32];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val[4];
      } rta_ipv4_vals[nested_ipv4];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);

    if (link->u.sittun.link != 0) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_IPTUN_LINK;
      rta.rta_u32_vals[nested_u32_idx].rta_val = link->u.sittun.link;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }

    if (link->u.sittun.local) {
      if (link->u.sittun.local->f.v4) {
        nested_ipv4_idx++;
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_IPTUN_LOCAL;
        memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
               link->u.sittun.local->v4.bytes, 4);
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
            sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
      }
    }
    if (link->u.sittun.remote) {
      if (link->u.sittun.remote->f.v4) {
        nested_ipv4_idx++;
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_type = IFLA_IPTUN_REMOTE;
        memcpy(rta.rta_ipv4_vals[nested_ipv4_idx].rta_val,
               link->u.sittun.remote->v4.bytes, 4);
        rta.rta_ipv4_vals[nested_ipv4_idx].rta_len =
            sizeof(rta.rta_ipv4_vals[nested_ipv4_idx]);
      }
    }
    if (link->u.sittun.ttl > 0) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_TTL;
      rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.sittun.ttl;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_TOS;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.sittun.tos;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u8_idx++;
    rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_IPTUN_PMTUDISC;
    rta.rta_u8_vals[nested_u8_idx].rta_val = (__u8)link->u.sittun.p_mtu_disc;
    rta.rta_u8_vals[nested_u8_idx].rta_len =
        sizeof(rta.rta_u8_vals[nested_u8_idx]);

    nested_u16_idx++;
    rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_TYPE;
    rta.rta_u16_vals[nested_u16_idx].rta_val = link->u.sittun.encap_type;
    rta.rta_u16_vals[nested_u16_idx].rta_len =
        sizeof(rta.rta_u16_vals[nested_u16_idx]);

    nested_u16_idx++;
    rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_FLAGS;
    rta.rta_u16_vals[nested_u16_idx].rta_val = link->u.sittun.encap_flags;
    rta.rta_u16_vals[nested_u16_idx].rta_len =
        sizeof(rta.rta_u16_vals[nested_u16_idx]);

    nested_u16_idx++;
    rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_SPORT;
    rta.rta_u16_vals[nested_u16_idx].rta_val =
        htons(link->u.sittun.encap_s_port);
    rta.rta_u16_vals[nested_u16_idx].rta_len =
        sizeof(rta.rta_u16_vals[nested_u16_idx]);

    nested_u16_idx++;
    rta.rta_u16_vals[nested_u16_idx].rta_type = IFLA_IPTUN_ENCAP_DPORT;
    rta.rta_u16_vals[nested_u16_idx].rta_val =
        htons(link->u.sittun.encap_d_port);
    rta.rta_u16_vals[nested_u16_idx].rta_len =
        sizeof(rta.rta_u16_vals[nested_u16_idx]);

    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  if (link->type.vti) {
    // TODO benne
  } else if (link->type.vrf) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_val;
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
    rta.rta_val.rta_type = IFLA_VRF_TABLE;
    rta.rta_val.rta_val = link->u.vrf.table;
    rta.rta_val.rta_len = sizeof(rta.rta_val);
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.bridge) {
    int nested_u8 = 0, nested_u8_idx = -1;
    int nested_u32 = 0, nested_u32_idx = -1;
    if (link->u.bridge.multicast_snooping) {
      nested_u8++;
    }
    if (link->u.bridge.hello_time) {
      nested_u32++;
    }
    if (link->u.bridge.vlan_filtering) {
      nested_u8++;
    }
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u8 rta_val;
      } rta_u8_vals[nested_u8];
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_u32_vals[nested_u32];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
    if (link->u.bridge.multicast_snooping) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BR_MCAST_SNOOPING;
      rta.rta_u8_vals[nested_u8_idx].rta_val =
          *link->u.bridge.multicast_snooping;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    if (link->u.bridge.hello_time) {
      nested_u32_idx++;
      rta.rta_u32_vals[nested_u32_idx].rta_type = IFLA_BR_MCAST_SNOOPING;
      rta.rta_u32_vals[nested_u32_idx].rta_val = *link->u.bridge.hello_time;
      rta.rta_u32_vals[nested_u32_idx].rta_len =
          sizeof(rta.rta_u32_vals[nested_u32_idx]);
    }
    if (link->u.bridge.vlan_filtering) {
      nested_u8_idx++;
      rta.rta_u8_vals[nested_u8_idx].rta_type = IFLA_BR_MCAST_SNOOPING;
      rta.rta_u8_vals[nested_u8_idx].rta_val = *link->u.bridge.vlan_filtering;
      rta.rta_u8_vals[nested_u8_idx].rta_len =
          sizeof(rta.rta_u8_vals[nested_u8_idx]);
    }
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.gtp) {
    int nested_attrs = 3;
    if (link->u.gtp.role != GTP_ROLE_GGSN) {
      nested_attrs = 4;
    }
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_vals[nested_attrs];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
    rta.rta_vals[0].rta_type = IFLA_GTP_FD0;
    rta.rta_vals[0].rta_val = link->u.gtp.fd0;
    rta.rta_vals[0].rta_len = sizeof(rta.rta_vals[0]);
    rta.rta_vals[1].rta_type = IFLA_GTP_FD1;
    rta.rta_vals[1].rta_val = link->u.gtp.fd1;
    rta.rta_vals[1].rta_len = sizeof(rta.rta_vals[1]);
    rta.rta_vals[2].rta_type = IFLA_GTP_PDP_HASHSIZE;
    rta.rta_vals[2].rta_val = (__u32)131072;
    rta.rta_vals[2].rta_len = sizeof(rta.rta_vals[2]);
    if (link->u.gtp.role != GTP_ROLE_GGSN) {
      rta.rta_vals[3].rta_type = IFLA_GTP_ROLE;
      rta.rta_vals[3].rta_val = link->u.gtp.role;
      rta.rta_vals[3].rta_len = sizeof(rta.rta_vals[3]);
    }
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.xfrm) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u32 rta_val;
      } rta_vals[2];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
    rta.rta_vals[0].rta_type = IFLA_XFRM_LINK;
    rta.rta_vals[0].rta_val = base.parent_index;
    rta.rta_vals[0].rta_len = sizeof(rta.rta_vals[0]);
    rta.rta_vals[1].rta_type = IFLA_XFRM_IF_ID;
    rta.rta_vals[1].rta_val = link->u.xfrm.ifid;
    rta.rta_vals[1].rta_len = sizeof(rta.rta_vals[1]);
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  } else if (link->type.ipoib) {
    struct {
      __u16 rta_len;
      __u16 rta_type;
      struct {
        __u16 rta_len;
        __u16 rta_type;
        __u16 rta_val;
      } rta_vals[3];
    } rta;
    memset(&rta, 0, sizeof(rta));
    rta.rta_type = IFLA_INFO_DATA;
    rta.rta_len = sizeof(rta);
    rta.rta_vals[0].rta_type = IFLA_IPOIB_PKEY;
    rta.rta_vals[0].rta_val = link->u.ipoib.pkey;
    rta.rta_vals[0].rta_len = sizeof(rta.rta_vals[0]);
    rta.rta_vals[1].rta_type = IFLA_IPOIB_MODE;
    rta.rta_vals[1].rta_val = link->u.ipoib.mode;
    rta.rta_vals[1].rta_len = sizeof(rta.rta_vals[1]);
    rta.rta_vals[2].rta_type = IFLA_IPOIB_UMCAST;
    rta.rta_vals[2].rta_val = link->u.ipoib.umcast;
    rta.rta_vals[2].rta_len = sizeof(rta.rta_vals[2]);
    ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
    if (ret < 0) {
      nlmsg_free(msg);
      nl_socket_free(socket);
      return false;
    }
  }

  ret = nl_send_auto_complete(socket, msg);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  nlmsg_free(msg);
  nl_socket_free(socket);

  return true;
}