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