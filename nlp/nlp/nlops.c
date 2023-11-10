#include <nlp.h>
#include <unistd.h>

nl_rtattr_t *nl_rtattr_alloc(void) {
  nl_rtattr_t *e;

  if (!(e = calloc(1, sizeof(*e)))) {
    return NULL;
  }

  NL_DBG(2, "allocated ematch %p\n", e);
  NL_INIT_LIST_HEAD(&e->nl_list);
  NL_INIT_LIST_HEAD(&e->children);

  return e;
}

void nl_rtattr_unlink(nl_rtattr_t *e) {
  nl_list_del(&e->nl_list);
  nl_init_list_head(&e->nl_list);
}

void nl_rtattr_free(nl_rtattr_t *e) {
  NL_DBG(2, "freed nl_rtattr_t %p\n", e);
  if (!nl_list_empty(&e->children)) {
    nl_rtattr_free_child(&e->children);
  }
  nl_rtattr_unlink(e);
  if (e->rta_val) {
    free(e->rta_val);
  }
  free(e);
}

void nl_rtattr_add_child(nl_rtattr_t *parent, nl_rtattr_t *child) {
  nl_list_add_tail(&child->nl_list, &parent->children);
}

void nl_rtattr_free_child(struct nl_list_head *head) {
  nl_rtattr_t *pos, *next;
  nl_list_for_each_entry_safe(pos, next, head, nl_list) {
    if (!nl_list_empty(&pos->children)) {
      nl_rtattr_free_child(&pos->children);
    }
    nl_rtattr_free(pos);
  }
}

__u16 nl_rtattr_len(nl_rtattr_t *e) {
  if (nl_list_empty(&e->children)) {
    if (e->rta_type > 0) {
      return RTA_ALIGN(4 + e->rta_val_len);
    } else {
      return RTA_ALIGN(e->rta_val_len);
    }
  }
  __u16 len = 0;
  struct nl_list_head *head = &e->children;
  if (!nl_list_empty(head)) {
    nl_rtattr_t *pos;
    nl_list_for_each_entry(pos, head, nl_list) {
      len += RTA_ALIGN(nl_rtattr_len(pos));
    }
  }
  if (e->rta_type > 0) {
    len += 4;
    return RTA_ALIGN(len + e->rta_val_len);
  } else {
    return RTA_ALIGN(len);
  }
}

__u8 *nl_rtattr_serialize(nl_rtattr_t *e, __u16 *length) {
  __u16 len = nl_rtattr_len(e);
  __u8 *buf = calloc(1, len);

  __u16 offset = 0;
  if (e->rta_type > 0) {
    offset = 4;
    if (e->rta_val_len > 0) {
      memcpy(buf + offset, e->rta_val, e->rta_val_len);
      offset += RTA_ALIGN(e->rta_val_len);
    }
  }

  struct nl_list_head *head = &e->children;
  if (!nl_list_empty(head)) {
    nl_rtattr_t *pos;
    nl_list_for_each_entry(pos, head, nl_list) {
      __u16 pos_len = 0;
      __u8 *pos_buf = nl_rtattr_serialize(pos, &pos_len);
      if (pos_len > 0) {
        memcpy(buf + offset, pos_buf, pos_len);
        offset += RTA_ALIGN(pos_len);
        free(pos_buf);
      }
    }
  }

  if (e->rta_type > 0) {
    if (len != 0) {
      memcpy(buf, &len, 2);
    }
    memcpy(buf + 2, &e->rta_type, 2);
  }

  *length = len;
  return buf;
}

nl_rtattr_t *nl_rtattr_new(__u16 rta_type, __u16 rta_val_len, __u8 *rta_val) {
  nl_rtattr_t *attr = nl_rtattr_alloc();
  attr->rta_type = rta_type;
  attr->rta_val_len = rta_val_len;
  if (rta_val_len > 0) {
    attr->rta_val = calloc(1, rta_val_len);
    memcpy(attr->rta_val, rta_val, rta_val_len);
  }
  return attr;
}

bool nl_route_add(const char *dst_str, const char *gw_str) {
  if (strlen(dst_str) == 0 || strlen(gw_str) == 0) {
    return false;
  }
  struct nl_ip_net dst;
  if (!parse_ip_net(dst_str, &dst, NULL) || (!dst.ip.f.v4 && !dst.ip.f.v6)) {
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
  if (!parse_ip_net(dst_str, &dst, NULL) || (!dst.ip.f.v4 && !dst.ip.f.v6)) {
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
  if (!parse_label_net(addr_str, &addr, NULL) ||
      (!addr.ip.f.v4 && !addr.ip.f.v6)) {
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

bool nl_vlan_add(int vlan_id) {
  char vlan_name[IF_NAMESIZE];
  memset(vlan_name, 0, IF_NAMESIZE);
  snprintf(vlan_name, IF_NAMESIZE, "vlan%d", vlan_id);
  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  nl_link_get_by_name(vlan_name, &port);
  if (port.index == 0) {
    nl_link_t link;
    memset(&link, 0, sizeof(link));
    link.attrs.name = vlan_name;
    link.attrs.mtu = 9000;
    link.type.bridge = 1;
    return nl_link_add(&link, NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
  }
  return false;
}

bool nl_vlan_del(int vlan_id) {
  char vlan_name[IF_NAMESIZE];
  memset(vlan_name, 0, IF_NAMESIZE);
  snprintf(vlan_name, IF_NAMESIZE, "vlan%d", vlan_id);
  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  nl_link_get_by_name(vlan_name, &port);
  if (port.index == 0) {
    return false;
  }
  if (!nl_link_down(port.index)) {
    return false;
  }
  return nl_link_del(port.index);
}

bool nl_vlan_member_add(int vlan_id, const char *ifi_name, bool tagged) {
  char vlan_bridge_name[IF_NAMESIZE];
  memset(vlan_bridge_name, 0, IF_NAMESIZE);
  snprintf(vlan_bridge_name, IF_NAMESIZE, "vlan%d", vlan_id);
  nl_port_mod_t vlan_bridge;
  memset(&vlan_bridge, 0, sizeof(vlan_bridge));
  nl_link_get_by_name(vlan_bridge_name, &vlan_bridge);
  if (vlan_bridge.index == 0) {
    return false;
  }
  nl_port_mod_t parent_link;
  memset(&parent_link, 0, sizeof(parent_link));
  nl_link_get_by_name(ifi_name, &parent_link);
  if (parent_link.index == 0) {
    return false;
  }

  char vlan_dev_name[IF_NAMESIZE];
  memset(vlan_dev_name, 0, IF_NAMESIZE);
  if (tagged) {
    snprintf(vlan_dev_name, IF_NAMESIZE, "%s.%d", ifi_name, vlan_id);
    nl_link_t vlan_link;
    memset(&vlan_link, 0, sizeof(vlan_link));
    vlan_link.attrs.name = vlan_dev_name;
    vlan_link.attrs.parent_index = (__s32)parent_link.index;
    vlan_link.type.vlan = 1;
    vlan_link.u.vlan.vlan_id = vlan_id;
    if (!nl_link_add(&vlan_link, NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK)) {
      return false;
    }
  } else {
    snprintf(vlan_dev_name, IF_NAMESIZE, "%s", ifi_name);
  }

  nl_port_mod_t vlan_dev_link;
  memset(&vlan_dev_link, 0, sizeof(vlan_dev_link));
  nl_link_get_by_name(vlan_dev_name, &vlan_dev_link);
  if (vlan_dev_link.index == 0) {
    return false;
  }
  nl_link_up(vlan_dev_link.index);
  return nl_link_master(vlan_dev_link.index, vlan_bridge.index);
}

bool nl_vlan_member_del(int vlan_id, const char *ifi_name, bool tagged) {
  char vlan_bridge_name[IF_NAMESIZE];
  memset(vlan_bridge_name, 0, IF_NAMESIZE);
  snprintf(vlan_bridge_name, IF_NAMESIZE, "vlan%d", vlan_id);
  nl_port_mod_t vlan_bridge;
  memset(&vlan_bridge, 0, sizeof(vlan_bridge));
  nl_link_get_by_name(vlan_bridge_name, &vlan_bridge);
  if (vlan_bridge.index == 0) {
    return false;
  }
  nl_port_mod_t parent_link;
  memset(&parent_link, 0, sizeof(parent_link));
  nl_link_get_by_name(ifi_name, &parent_link);
  if (parent_link.index == 0) {
    return false;
  }

  char vlan_dev_name[IF_NAMESIZE];
  memset(vlan_dev_name, 0, IF_NAMESIZE);
  if (tagged) {
    snprintf(vlan_dev_name, IF_NAMESIZE, "%s.%d", ifi_name, vlan_id);
  } else {
    snprintf(vlan_dev_name, IF_NAMESIZE, "%s", ifi_name);
  }

  nl_port_mod_t vlan_dev_link;
  memset(&vlan_dev_link, 0, sizeof(vlan_dev_link));
  nl_link_get_by_name(vlan_dev_name, &vlan_dev_link);
  if (vlan_dev_link.index == 0) {
    return false;
  }

  bool ret = nl_link_no_master(vlan_dev_link.index);
  if (ret && tagged) {
    return nl_link_del(vlan_dev_link.index);
  }
  return ret;
}

bool nl_vxlan_bridge_add(int vxlan_id, const char *ep_ifi_name) {
  char vxlan_bridge_name[IF_NAMESIZE];
  memset(vxlan_bridge_name, 0, IF_NAMESIZE);
  snprintf(vxlan_bridge_name, IF_NAMESIZE, "vxlan%d", vxlan_id);
  nl_port_mod_t vxlan_bridge;
  memset(&vxlan_bridge, 0, sizeof(vxlan_bridge));
  nl_link_get_by_name(vxlan_bridge_name, &vxlan_bridge);
  if (vxlan_bridge.index > 0) {
    return false;
  }

  nl_port_mod_t endpoint_link;
  memset(&endpoint_link, 0, sizeof(endpoint_link));
  nl_link_get_by_name(ep_ifi_name, &endpoint_link);
  if (endpoint_link.index == 0) {
    return false;
  }

  ip_t src_addr;
  memset(&src_addr, 0, sizeof(src_addr));
  int addrs_cnt = 0;
  nl_addr_mod_t *addrs =
      nl_link_addr_list(endpoint_link.index, FAMILY_V4, &addrs_cnt);
  if (addrs_cnt == 0) {
    free(addrs);
    return false;
  } else {
    memcpy(&src_addr, &addrs[0].ipnet, sizeof(src_addr));
    free(addrs);
  }

  nl_link_t vxlan_link;
  memset(&vxlan_link, 0, sizeof(vxlan_link));
  vxlan_link.attrs.name = vxlan_bridge_name;
  vxlan_link.attrs.mtu = 9000;
  vxlan_link.type.vxlan = 1;
  vxlan_link.u.vxlan.vxlan_id = vxlan_id;
  vxlan_link.u.vxlan.vtep_dev_index = endpoint_link.index;
  vxlan_link.u.vxlan.port = 4789; // VxLAN default port
  vxlan_link.u.vxlan.src_addr = &src_addr;

  if (!nl_link_add(&vxlan_link, NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK)) {
    return false;
  }

  sleep(1);

  memset(&vxlan_bridge, 0, sizeof(vxlan_bridge));
  nl_link_get_by_name(vxlan_bridge_name, &vxlan_bridge);
  if (vxlan_bridge.index == 0) {
    return false;
  }

  return nl_link_up(vxlan_bridge.index);
}

bool nl_vxlan_del(int vxlan_id) {
  char vxlan_bridge_name[IF_NAMESIZE];
  memset(vxlan_bridge_name, 0, IF_NAMESIZE);
  snprintf(vxlan_bridge_name, IF_NAMESIZE, "vxlan%d", vxlan_id);
  nl_port_mod_t vxlan_bridge;
  memset(&vxlan_bridge, 0, sizeof(vxlan_bridge));
  nl_link_get_by_name(vxlan_bridge_name, &vxlan_bridge);
  if (vxlan_bridge.index == 0) {
    return false;
  }

  if (!nl_link_down(vxlan_bridge.index)) {
    return false;
  }

  return nl_link_del(vxlan_bridge.index);
}

#define IFF_TUN 0x1
#define IFF_TAP 0x2
#define VETH_INFO_PEER 1
#define IFLA_VXLAN_FLOWBASED 25

bool nl_link_add(nl_link_t *link, int flags) {
  nl_base_attrs_t base = link->attrs;

  if ((!base.name || strlen(base.name) == 0) && !link->type.tun) {
    return false;
  }

  if (link->type.tun) {
    if (link->u.tuntap.mode < IFF_TUN || link->u.tuntap.mode > IFF_TAP) {
      return false;
    }
    // TODO benne
    return false;
  }

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
  if (!type) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
  }

  nl_rtattr_t *link_info = nl_rtattr_new(IFLA_LINKINFO, 0, NULL);

  nl_rtattr_t *link_info_kind =
      nl_rtattr_new(IFLA_INFO_KIND, strlen(type), (__u8 *)type);
  nl_rtattr_add_child(link_info, link_info_kind);

  if (link->type.vlan) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_vlan_id =
        nl_rtattr_new(IFLA_VLAN_ID, 2, (__u8 *)&link->u.vlan.vlan_id);
    nl_rtattr_add_child(link_info_data, attr_vlan_id);

    if (link->u.vlan.vlan_protocol != 0) {
      __u16 vlan_protocol = htons(link->u.vlan.vlan_protocol);
      nl_rtattr_t *attr_vlan_protocol =
          nl_rtattr_new(IFLA_VLAN_PROTOCOL, 2, (__u8 *)&vlan_protocol);
      nl_rtattr_add_child(link_info_data, attr_vlan_protocol);
    }
  } else if (link->type.veth) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_peer = nl_rtattr_new(VETH_INFO_PEER, 0, NULL);
    nl_rtattr_add_child(link_info_data, attr_peer);

    struct ifinfomsg ifi_msg;
    memset(&ifi_msg, 0, sizeof(ifi_msg));
    ifi_msg.ifi_family = AF_UNSPEC;
    nl_rtattr_t *attr_ifi_msg =
        nl_rtattr_new(0, sizeof(ifi_msg), (__u8 *)&ifi_msg);
    nl_rtattr_add_child(attr_peer, attr_ifi_msg);

    __u8 ifname[strlen(link->u.veth.peer_name) + 1];
    memset(ifname, 0, strlen(link->u.veth.peer_name) + 1);
    memcpy(ifname, link->u.veth.peer_name, strlen(link->u.veth.peer_name));
    nl_rtattr_t *attr_ifname = nl_rtattr_new(
        IFLA_IFNAME, strlen(link->u.veth.peer_name) + 1, (__u8 *)ifname);
    nl_rtattr_add_child(attr_peer, attr_ifname);

    if (base.tx_q_len >= 0) {
      nl_rtattr_t *attr_tx_q_len =
          nl_rtattr_new(IFLA_TXQLEN, 4, (__u8 *)&base.tx_q_len);
      nl_rtattr_add_child(attr_peer, attr_tx_q_len);
    }

    if (base.num_tx_queues > 0) {
      nl_rtattr_t *attr_num_tx_queues =
          nl_rtattr_new(IFLA_NUM_TX_QUEUES, 4, (__u8 *)&base.num_tx_queues);
      nl_rtattr_add_child(attr_peer, attr_num_tx_queues);
    }

    if (base.num_rx_queues > 0) {
      nl_rtattr_t *attr_num_rx_queues =
          nl_rtattr_new(IFLA_NUM_RX_QUEUES, 4, (__u8 *)&base.num_rx_queues);
      nl_rtattr_add_child(attr_peer, attr_num_rx_queues);
    }

    if (base.mtu > 0) {
      nl_rtattr_t *attr_mtu = nl_rtattr_new(IFLA_MTU, 4, (__u8 *)&base.mtu);
      nl_rtattr_add_child(attr_peer, attr_mtu);
    }

    if (!is_zero_mac(link->u.veth.peer_hw_addr)) {
      nl_rtattr_t *attr_hw_address = nl_rtattr_new(
          IFLA_ADDRESS, ETH_ALEN, (__u8 *)link->u.veth.peer_hw_addr);
      nl_rtattr_add_child(attr_peer, attr_hw_address);
    }
  } else if (link->type.vxlan) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.vxlan.flow_based) {
      __u32 vxlan_id = 0;
      nl_rtattr_t *attr_vlan_id =
          nl_rtattr_new(IFLA_VLAN_ID, 2, (__u8 *)&vxlan_id);
      nl_rtattr_add_child(link_info_data, attr_vlan_id);
    } else {
      nl_rtattr_t *attr_vxlan_id =
          nl_rtattr_new(IFLA_VXLAN_ID, 2, (__u8 *)&link->u.vxlan.vxlan_id);
      nl_rtattr_add_child(link_info_data, attr_vxlan_id);
    }

    if (link->u.vxlan.vtep_dev_index != 0) {
      nl_rtattr_t *attr_vxlan_link = nl_rtattr_new(
          IFLA_VXLAN_LINK, 2, (__u8 *)&link->u.vxlan.vtep_dev_index);
      nl_rtattr_add_child(link_info_data, attr_vxlan_link);
    }

    if (link->u.vxlan.src_addr) {
      if (link->u.vxlan.src_addr->f.v4) {
        nl_rtattr_t *attr_vxlan_local = nl_rtattr_new(
            IFLA_VXLAN_LOCAL, 4, (__u8 *)link->u.vxlan.src_addr->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_vxlan_local);
      } else if (link->u.vxlan.src_addr->f.v6) {
        nl_rtattr_t *attr_vxlan_local6 = nl_rtattr_new(
            IFLA_VXLAN_LOCAL6, 16, (__u8 *)link->u.vxlan.src_addr->v6.bytes);
        nl_rtattr_add_child(link_info_data, attr_vxlan_local6);
      }
    }

    if (link->u.vxlan.group) {
      if (link->u.vxlan.group->f.v4) {
        nl_rtattr_t *attr_vxlan_group = nl_rtattr_new(
            IFLA_VXLAN_GROUP, 4, (__u8 *)link->u.vxlan.group->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_vxlan_group);
      } else if (link->u.vxlan.group->f.v6) {
        nl_rtattr_t *attr_vxlan_group6 = nl_rtattr_new(
            IFLA_VXLAN_GROUP6, 16, (__u8 *)link->u.vxlan.group->v6.bytes);
        nl_rtattr_add_child(link_info_data, attr_vxlan_group6);
      }
    }

    nl_rtattr_t *attr_vxlan_ttl =
        nl_rtattr_new(IFLA_VXLAN_TTL, 1, (__u8 *)&link->u.vxlan.ttl);
    nl_rtattr_add_child(link_info_data, attr_vxlan_ttl);

    nl_rtattr_t *attr_vxlan_tos =
        nl_rtattr_new(IFLA_VXLAN_TOS, 1, (__u8 *)&link->u.vxlan.tos);
    nl_rtattr_add_child(link_info_data, attr_vxlan_tos);

    nl_rtattr_t *attr_vxlan_learning =
        nl_rtattr_new(IFLA_VXLAN_LEARNING, 1, (__u8 *)&link->u.vxlan.learning);
    nl_rtattr_add_child(link_info_data, attr_vxlan_learning);

    nl_rtattr_t *attr_vxlan_proxy =
        nl_rtattr_new(IFLA_VXLAN_PROXY, 1, (__u8 *)&link->u.vxlan.proxy);
    nl_rtattr_add_child(link_info_data, attr_vxlan_proxy);

    nl_rtattr_t *attr_vxlan_rsc =
        nl_rtattr_new(IFLA_VXLAN_RSC, 1, (__u8 *)&link->u.vxlan.rsc);
    nl_rtattr_add_child(link_info_data, attr_vxlan_rsc);

    nl_rtattr_t *attr_vxlan_l2_miss =
        nl_rtattr_new(IFLA_VXLAN_L2MISS, 1, (__u8 *)&link->u.vxlan.l2_miss);
    nl_rtattr_add_child(link_info_data, attr_vxlan_l2_miss);

    nl_rtattr_t *attr_vxlan_l3_miss =
        nl_rtattr_new(IFLA_VXLAN_L3MISS, 1, (__u8 *)&link->u.vxlan.l3_miss);
    nl_rtattr_add_child(link_info_data, attr_vxlan_l3_miss);

    nl_rtattr_t *attr_vxlan_udp6_zero_csum_tx =
        nl_rtattr_new(IFLA_VXLAN_UDP_ZERO_CSUM6_TX, 1,
                      (__u8 *)&link->u.vxlan.udp6_zero_csum_tx);
    nl_rtattr_add_child(link_info_data, attr_vxlan_udp6_zero_csum_tx);

    nl_rtattr_t *attr_vxlan_udp6_zero_csum_rx =
        nl_rtattr_new(IFLA_VXLAN_UDP_ZERO_CSUM6_RX, 1,
                      (__u8 *)&link->u.vxlan.udp6_zero_csum_rx);
    nl_rtattr_add_child(link_info_data, attr_vxlan_udp6_zero_csum_rx);

    if (link->u.vxlan.udp_csum) {
      nl_rtattr_t *attr_vxlan_udp_csum = nl_rtattr_new(
          IFLA_VXLAN_UDP_CSUM, 1, (__u8 *)&link->u.vxlan.udp_csum);
      nl_rtattr_add_child(link_info_data, attr_vxlan_udp_csum);
    }

    if (link->u.vxlan.flow_based) {
      nl_rtattr_t *attr_vxlan_flow_based = nl_rtattr_new(
          IFLA_VXLAN_FLOWBASED, 1, (__u8 *)&link->u.vxlan.flow_based);
      nl_rtattr_add_child(link_info_data, attr_vxlan_flow_based);
    }

    if (link->u.vxlan.no_age) {
      __u32 age = 0;
      nl_rtattr_t *attr_vxlan_age =
          nl_rtattr_new(IFLA_VXLAN_AGEING, 4, (__u8 *)&age);
      nl_rtattr_add_child(link_info_data, attr_vxlan_age);
    } else if (link->u.vxlan.age > 0) {
      nl_rtattr_t *attr_vxlan_age =
          nl_rtattr_new(IFLA_VXLAN_AGEING, 4, (__u8 *)&link->u.vxlan.age);
      nl_rtattr_add_child(link_info_data, attr_vxlan_age);
    }

    if (link->u.vxlan.limit > 0) {
      nl_rtattr_t *attr_vxlan_limit =
          nl_rtattr_new(IFLA_VXLAN_LIMIT, 4, (__u8 *)&link->u.vxlan.limit);
      nl_rtattr_add_child(link_info_data, attr_vxlan_limit);
    }

    if (link->u.vxlan.port > 0) {
      __u16 port = htons(link->u.vxlan.port);
      nl_rtattr_t *attr_vxlan_port =
          nl_rtattr_new(IFLA_VXLAN_PORT, 4, (__u8 *)&port);
      nl_rtattr_add_child(link_info_data, attr_vxlan_port);
    }

    if (link->u.vxlan.port_low > 0 || link->u.vxlan.port_high > 0) {
      __u32 port_range =
          htons(link->u.vxlan.port_low) << 16 | htons(link->u.vxlan.port_high);
      nl_rtattr_t *attr_vxlan_port_range =
          nl_rtattr_new(IFLA_VXLAN_PORT_RANGE, 4, (__u8 *)&port_range);
      nl_rtattr_add_child(link_info_data, attr_vxlan_port_range);
    }

    if (link->u.vxlan.gbp) {
      nl_rtattr_t *attr_vxlan_gbp = nl_rtattr_new(IFLA_VXLAN_GBP, 0, NULL);
      nl_rtattr_add_child(link_info_data, attr_vxlan_gbp);
    }
  } else if (link->type.bond) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.bond.mode >= 0) {
      nl_rtattr_t *attr_bond_mode =
          nl_rtattr_new(IFLA_BOND_MODE, 1, (__u8 *)&link->u.bond.mode);
      nl_rtattr_add_child(link_info_data, attr_bond_mode);
    }
    if (link->u.bond.activeslave >= 0) {
      nl_rtattr_t *attr_bond_activeslave = nl_rtattr_new(
          IFLA_BOND_ACTIVE_SLAVE, 4, (__u8 *)&link->u.bond.activeslave);
      nl_rtattr_add_child(link_info_data, attr_bond_activeslave);
    }
    if (link->u.bond.miimon >= 0) {
      nl_rtattr_t *attr_bond_miimon =
          nl_rtattr_new(IFLA_BOND_MIIMON, 4, (__u8 *)&link->u.bond.miimon);
      nl_rtattr_add_child(link_info_data, attr_bond_miimon);
    }
    if (link->u.bond.up_delay >= 0) {
      nl_rtattr_t *attr_bond_up_delay =
          nl_rtattr_new(IFLA_BOND_UPDELAY, 4, (__u8 *)&link->u.bond.up_delay);
      nl_rtattr_add_child(link_info_data, attr_bond_up_delay);
    }
    if (link->u.bond.down_delay >= 0) {
      nl_rtattr_t *attr_bond_down_delay = nl_rtattr_new(
          IFLA_BOND_DOWNDELAY, 4, (__u8 *)&link->u.bond.down_delay);
      nl_rtattr_add_child(link_info_data, attr_bond_down_delay);
    }
    if (link->u.bond.use_carrier >= 0) {
      nl_rtattr_t *attr_bond_use_carrier = nl_rtattr_new(
          IFLA_BOND_USE_CARRIER, 1, (__u8 *)&link->u.bond.use_carrier);
      nl_rtattr_add_child(link_info_data, attr_bond_use_carrier);
    }
    if (link->u.bond.arp_interval >= 0) {
      nl_rtattr_t *attr_bond_arp_interval = nl_rtattr_new(
          IFLA_BOND_ARP_INTERVAL, 4, (__u8 *)&link->u.bond.arp_interval);
      nl_rtattr_add_child(link_info_data, attr_bond_arp_interval);
    }
    if (link->u.bond.arp_validate >= 0) {
      nl_rtattr_t *attr_bond_arp_validate = nl_rtattr_new(
          IFLA_BOND_ARP_VALIDATE, 4, (__u8 *)&link->u.bond.arp_validate);
      nl_rtattr_add_child(link_info_data, attr_bond_arp_validate);
    }
    if (link->u.bond.arp_all_targets >= 0) {
      nl_rtattr_t *attr_bond_arp_all_targets = nl_rtattr_new(
          IFLA_BOND_ARP_ALL_TARGETS, 4, (__u8 *)&link->u.bond.arp_all_targets);
      nl_rtattr_add_child(link_info_data, attr_bond_arp_all_targets);
    }
    if (link->u.bond.primary >= 0) {
      nl_rtattr_t *attr_bond_primary =
          nl_rtattr_new(IFLA_BOND_PRIMARY, 4, (__u8 *)&link->u.bond.primary);
      nl_rtattr_add_child(link_info_data, attr_bond_primary);
    }
    if (link->u.bond.primary_reselect >= 0) {
      nl_rtattr_t *attr_bond_primary_reselect =
          nl_rtattr_new(IFLA_BOND_PRIMARY_RESELECT, 1,
                        (__u8 *)&link->u.bond.primary_reselect);
      nl_rtattr_add_child(link_info_data, attr_bond_primary_reselect);
    }
    if (link->u.bond.failover_mac >= 0) {
      nl_rtattr_t *attr_bond_failover_mac = nl_rtattr_new(
          IFLA_BOND_FAIL_OVER_MAC, 1, (__u8 *)&link->u.bond.failover_mac);
      nl_rtattr_add_child(link_info_data, attr_bond_failover_mac);
    }
    if (link->u.bond.xmit_hash_policy >= 0) {
      nl_rtattr_t *attr_bond_xmit_hash_policy =
          nl_rtattr_new(IFLA_BOND_XMIT_HASH_POLICY, 1,
                        (__u8 *)&link->u.bond.xmit_hash_policy);
      nl_rtattr_add_child(link_info_data, attr_bond_xmit_hash_policy);
    }
    if (link->u.bond.resend_igmp >= 0) {
      nl_rtattr_t *attr_bond_resend_igmp = nl_rtattr_new(
          IFLA_BOND_RESEND_IGMP, 4, (__u8 *)&link->u.bond.resend_igmp);
      nl_rtattr_add_child(link_info_data, attr_bond_resend_igmp);
    }
    if (link->u.bond.num_peer_notif >= 0) {
      nl_rtattr_t *attr_bond_num_peer_notif = nl_rtattr_new(
          IFLA_BOND_NUM_PEER_NOTIF, 1, (__u8 *)&link->u.bond.num_peer_notif);
      nl_rtattr_add_child(link_info_data, attr_bond_num_peer_notif);
    }
    if (link->u.bond.all_slaves_active >= 0) {
      nl_rtattr_t *attr_bond_all_slaves_active =
          nl_rtattr_new(IFLA_BOND_ALL_SLAVES_ACTIVE, 1,
                        (__u8 *)&link->u.bond.all_slaves_active);
      nl_rtattr_add_child(link_info_data, attr_bond_all_slaves_active);
    }
    if (link->u.bond.min_links >= 0) {
      nl_rtattr_t *attr_bond_min_links = nl_rtattr_new(
          IFLA_BOND_MIN_LINKS, 4, (__u8 *)&link->u.bond.min_links);
      nl_rtattr_add_child(link_info_data, attr_bond_min_links);
    }
    if (link->u.bond.lp_interval >= 0) {
      nl_rtattr_t *attr_bond_lp_interval = nl_rtattr_new(
          IFLA_BOND_LP_INTERVAL, 4, (__u8 *)&link->u.bond.lp_interval);
      nl_rtattr_add_child(link_info_data, attr_bond_lp_interval);
    }
    if (link->u.bond.packers_per_slave >= 0) {
      nl_rtattr_t *attr_bond_packers_per_slave =
          nl_rtattr_new(IFLA_BOND_PACKETS_PER_SLAVE, 4,
                        (__u8 *)&link->u.bond.packers_per_slave);
      nl_rtattr_add_child(link_info_data, attr_bond_packers_per_slave);
    }
    if (link->u.bond.lacp_rate >= 0) {
      nl_rtattr_t *attr_bond_lacp_rate = nl_rtattr_new(
          IFLA_BOND_AD_LACP_RATE, 1, (__u8 *)&link->u.bond.lacp_rate);
      nl_rtattr_add_child(link_info_data, attr_bond_lacp_rate);
    }
    if (link->u.bond.ad_select >= 0) {
      nl_rtattr_t *attr_bond_ad_select = nl_rtattr_new(
          IFLA_BOND_AD_SELECT, 1, (__u8 *)&link->u.bond.ad_select);
      nl_rtattr_add_child(link_info_data, attr_bond_ad_select);
    }
    if (link->u.bond.ad_actor_sys_prio >= 0) {
      nl_rtattr_t *attr_bond_ad_actor_sys_prio =
          nl_rtattr_new(IFLA_BOND_AD_ACTOR_SYS_PRIO, 2,
                        (__u8 *)&link->u.bond.ad_actor_sys_prio);
      nl_rtattr_add_child(link_info_data, attr_bond_ad_actor_sys_prio);
    }
    if (link->u.bond.ad_user_port_key >= 0) {
      nl_rtattr_t *attr_bond_ad_user_port_key =
          nl_rtattr_new(IFLA_BOND_AD_USER_PORT_KEY, 2,
                        (__u8 *)&link->u.bond.ad_user_port_key);
      nl_rtattr_add_child(link_info_data, attr_bond_ad_user_port_key);
    }
    if (link->u.bond.tlb_dynamic_lb >= 0) {
      nl_rtattr_t *attr_bond_tlb_dynamic_lb = nl_rtattr_new(
          IFLA_BOND_TLB_DYNAMIC_LB, 1, (__u8 *)&link->u.bond.tlb_dynamic_lb);
      nl_rtattr_add_child(link_info_data, attr_bond_tlb_dynamic_lb);
    }
    if (!is_zero_mac(link->u.bond.ad_actor_system)) {
      nl_rtattr_t *attr_bond_ad_actor_system =
          nl_rtattr_new(IFLA_BOND_AD_ACTOR_SYSTEM, ETH_ALEN,
                        (__u8 *)&link->u.bond.ad_actor_system);
      nl_rtattr_add_child(link_info_data, attr_bond_ad_actor_system);
    }

    if (link->u.bond.arp_ip_targets) {
      nl_rtattr_t *attr_bond_arp_ip_target =
          nl_rtattr_new(IFLA_BOND_ARP_IP_TARGET, 0, NULL);
      nl_rtattr_add_child(link_info_data, attr_bond_arp_ip_target);
      int num = sizeof(*link->u.bond.arp_ip_targets) / sizeof(ip_t);
      for (int i = 0; i < num; i++) {
        if (link->u.bond.arp_ip_targets[i].f.v4) {
          nl_rtattr_t *attr_bond_arp_ipv4_target = nl_rtattr_new(
              i, 4, (__u8 *)link->u.bond.arp_ip_targets[i].v4.bytes);
          nl_rtattr_add_child(attr_bond_arp_ip_target,
                              attr_bond_arp_ipv4_target);
        } else if (link->u.bond.arp_ip_targets[i].f.v6) {
          nl_rtattr_t *attr_bond_arp_ipv6_target = nl_rtattr_new(
              i, 16, (__u8 *)link->u.bond.arp_ip_targets[i].v6.bytes);
          nl_rtattr_add_child(attr_bond_arp_ip_target,
                              attr_bond_arp_ipv6_target);
        }
      }
    }
  } else if (link->type.ipvlan) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_ipvlan_mode =
        nl_rtattr_new(IFLA_IPVLAN_MODE, 2, (__u8 *)&link->u.ipvlan.mode);
    nl_rtattr_add_child(link_info_data, attr_ipvlan_mode);

    nl_rtattr_t *attr_ipvlan_flag =
        nl_rtattr_new(IFLA_IPVLAN_FLAGS, 2, (__u8 *)&link->u.ipvlan.flag);
    nl_rtattr_add_child(link_info_data, attr_ipvlan_flag);
  } else if (link->type.ipvtap) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_ipvtap_mode =
        nl_rtattr_new(IFLA_IPVLAN_MODE, 2, (__u8 *)&link->u.ipvtap.mode);
    nl_rtattr_add_child(link_info_data, attr_ipvtap_mode);

    nl_rtattr_t *attr_ipvtap_flag =
        nl_rtattr_new(IFLA_IPVLAN_FLAGS, 2, (__u8 *)&link->u.ipvtap.flag);
    nl_rtattr_add_child(link_info_data, attr_ipvtap_flag);
  } else if (link->type.macvlan) {
    if (link->u.macvlan.mode != 0) {
      nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
      nl_rtattr_add_child(link_info, link_info_data);

      nl_rtattr_t *attr_macvlan_mode =
          nl_rtattr_new(IFLA_MACVLAN_MODE, 4, (__u8 *)&link->u.macvlan.mode);
      nl_rtattr_add_child(link_info_data, attr_macvlan_mode);
    }
  } else if (link->type.macvtap) {
    if (link->u.macvtap.mode != 0) {
      nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
      nl_rtattr_add_child(link_info, link_info_data);

      nl_rtattr_t *attr_macvtap_mode =
          nl_rtattr_new(IFLA_MACVLAN_MODE, 4, (__u8 *)&link->u.macvtap.mode);
      nl_rtattr_add_child(link_info_data, attr_macvtap_mode);
    }
  } else if (link->type.geneve) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.geneve.flow_based) {
      nl_rtattr_t *attr_geneve_flow_based = nl_rtattr_new(
          IFLA_GENEVE_COLLECT_METADATA, 1, (__u8 *)&link->u.geneve.flow_based);
      nl_rtattr_add_child(link_info_data, attr_geneve_flow_based);
    } else {
      if (link->u.geneve.remote) {
        if (link->u.geneve.remote->f.v4) {
          nl_rtattr_t *attr_geneve_remote = nl_rtattr_new(
              IFLA_GENEVE_REMOTE, 4, (__u8 *)link->u.geneve.remote->v4.bytes);
          nl_rtattr_add_child(link_info_data, attr_geneve_remote);
        } else if (link->u.geneve.remote->f.v6) {
          nl_rtattr_t *attr_geneve_remote = nl_rtattr_new(
              IFLA_GENEVE_REMOTE6, 16, (__u8 *)link->u.geneve.remote->v6.bytes);
          nl_rtattr_add_child(link_info_data, attr_geneve_remote);
        }
      }
      if (link->u.geneve.id != 0) {
        nl_rtattr_t *attr_geneve_id =
            nl_rtattr_new(IFLA_GENEVE_ID, 4, (__u8 *)&link->u.geneve.id);
        nl_rtattr_add_child(link_info_data, attr_geneve_id);
      }
      if (link->u.geneve.d_port != 0) {
        __u16 d_port = htons(link->u.geneve.d_port);
        nl_rtattr_t *attr_geneve_d_port =
            nl_rtattr_new(IFLA_GENEVE_PORT, 2, (__u8 *)&d_port);
        nl_rtattr_add_child(link_info_data, attr_geneve_d_port);
      }
      if (link->u.geneve.ttl != 0) {
        nl_rtattr_t *attr_geneve_ttl =
            nl_rtattr_new(IFLA_GENEVE_TTL, 1, (__u8 *)&link->u.geneve.ttl);
        nl_rtattr_add_child(link_info_data, attr_geneve_ttl);
      }
      if (link->u.geneve.tos != 0) {
        nl_rtattr_t *attr_geneve_tos =
            nl_rtattr_new(IFLA_GENEVE_TOS, 1, (__u8 *)&link->u.geneve.tos);
        nl_rtattr_add_child(link_info_data, attr_geneve_tos);
      }
    }
  } else if (link->type.gretap) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.gretap.flow_based) {
      nl_rtattr_t *attr_gretap_flow_based = nl_rtattr_new(
          IFLA_GRE_COLLECT_METADATA, 1, (__u8 *)&link->u.gretap.flow_based);
      nl_rtattr_add_child(link_info_data, attr_gretap_flow_based);
    } else {
      if (link->u.gretap.local) {
        if (link->u.gretap.local->f.v4) {
          nl_rtattr_t *attr_gretap_local = nl_rtattr_new(
              IFLA_GRE_LOCAL, 4, (__u8 *)link->u.gretap.local->v4.bytes);
          nl_rtattr_add_child(link_info_data, attr_gretap_local);
        }
      }
      if (link->u.gretap.remote) {
        if (link->u.gretap.remote->f.v4) {
          nl_rtattr_t *attr_gretap_remote = nl_rtattr_new(
              IFLA_GRE_REMOTE, 4, (__u8 *)link->u.gretap.remote->v4.bytes);
          nl_rtattr_add_child(link_info_data, attr_gretap_remote);
        }
      }
      if (link->u.gretap.ikey != 0) {
        __u32 ikey = htonl(link->u.gretap.ikey);
        nl_rtattr_t *attr_gretap_ikey =
            nl_rtattr_new(IFLA_GRE_IKEY, 4, (__u8 *)&ikey);
        nl_rtattr_add_child(link_info_data, attr_gretap_ikey);
        link->u.gretap.iflags |= GRE_KEY;
      }
      if (link->u.gretap.okey != 0) {
        __u32 okey = htonl(link->u.gretap.okey);
        nl_rtattr_t *attr_gretap_okey =
            nl_rtattr_new(IFLA_GRE_OKEY, 4, (__u8 *)&okey);
        nl_rtattr_add_child(link_info_data, attr_gretap_okey);
        link->u.gretap.oflags |= GRE_KEY;
      }

      __u16 iflags = htonl(link->u.gretap.iflags);
      nl_rtattr_t *attr_gretap_iflags =
          nl_rtattr_new(IFLA_GRE_IFLAGS, 4, (__u8 *)&iflags);
      nl_rtattr_add_child(link_info_data, attr_gretap_iflags);

      __u16 oflags = htonl(link->u.gretap.oflags);
      nl_rtattr_t *attr_gretap_oflags =
          nl_rtattr_new(IFLA_GRE_OFLAGS, 4, (__u8 *)&oflags);
      nl_rtattr_add_child(link_info_data, attr_gretap_oflags);

      if (link->u.gretap.link != 0) {
        nl_rtattr_t *attr_gretap_link =
            nl_rtattr_new(IFLA_GRE_LINK, 4, (__u8 *)&link->u.gretap.link);
        nl_rtattr_add_child(link_info_data, attr_gretap_link);
      }

      nl_rtattr_t *attr_gretap_p_mtu_disc = nl_rtattr_new(
          IFLA_GRE_PMTUDISC, 1, (__u8 *)&link->u.gretap.p_mtu_disc);
      nl_rtattr_add_child(link_info_data, attr_gretap_p_mtu_disc);

      nl_rtattr_t *attr_gretap_ttl =
          nl_rtattr_new(IFLA_GRE_TTL, 1, (__u8 *)&link->u.gretap.ttl);
      nl_rtattr_add_child(link_info_data, attr_gretap_ttl);

      nl_rtattr_t *attr_gretap_tos =
          nl_rtattr_new(IFLA_GRE_TOS, 1, (__u8 *)&link->u.gretap.tos);
      nl_rtattr_add_child(link_info_data, attr_gretap_tos);

      nl_rtattr_t *attr_gretap_encap_type = nl_rtattr_new(
          IFLA_GRE_ENCAP_TYPE, 2, (__u8 *)&link->u.gretap.encap_type);
      nl_rtattr_add_child(link_info_data, attr_gretap_encap_type);

      nl_rtattr_t *attr_gretap_encap_flags = nl_rtattr_new(
          IFLA_GRE_ENCAP_FLAGS, 2, (__u8 *)&link->u.gretap.encap_flags);
      nl_rtattr_add_child(link_info_data, attr_gretap_encap_flags);

      nl_rtattr_t *attr_gretap_encap_s_port = nl_rtattr_new(
          IFLA_GRE_ENCAP_SPORT, 2, (__u8 *)&link->u.gretap.encap_s_port);
      nl_rtattr_add_child(link_info_data, attr_gretap_encap_s_port);

      nl_rtattr_t *attr_gretap_encap_d_port = nl_rtattr_new(
          IFLA_GRE_ENCAP_DPORT, 2, (__u8 *)&link->u.gretap.encap_d_port);
      nl_rtattr_add_child(link_info_data, attr_gretap_encap_d_port);
    }
  } else if (link->type.ipip) {
    if (link->u.iptun.flow_based) {
      nl_rtattr_t *attr_iptun_flow_based = nl_rtattr_new(
          IFLA_IPTUN_COLLECT_METADATA, 1, (__u8 *)&link->u.iptun.flow_based);
      nl_rtattr_add_child(link_info, attr_iptun_flow_based);
    } else {
      nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
      nl_rtattr_add_child(link_info, link_info_data);

      if (link->u.iptun.local) {
        if (link->u.iptun.local->f.v4) {
          nl_rtattr_t *attr_iptun_local = nl_rtattr_new(
              IFLA_IPTUN_LOCAL, 4, (__u8 *)link->u.iptun.local->v4.bytes);
          nl_rtattr_add_child(link_info_data, attr_iptun_local);
        }
      }
      if (link->u.iptun.remote) {
        if (link->u.iptun.remote->f.v4) {
          nl_rtattr_t *attr_iptun_remote = nl_rtattr_new(
              IFLA_IPTUN_REMOTE, 4, (__u8 *)link->u.iptun.remote->v4.bytes);
          nl_rtattr_add_child(link_info_data, attr_iptun_remote);
        }
      }

      if (link->u.iptun.link != 0) {
        nl_rtattr_t *attr_iptun_link =
            nl_rtattr_new(IFLA_IPTUN_LINK, 4, (__u8 *)&link->u.iptun.link);
        nl_rtattr_add_child(link_info_data, attr_iptun_link);
      }

      nl_rtattr_t *attr_iptun_p_mtu_disc = nl_rtattr_new(
          IFLA_IPTUN_PMTUDISC, 1, (__u8 *)&link->u.iptun.p_mtu_disc);
      nl_rtattr_add_child(link_info_data, attr_iptun_p_mtu_disc);

      nl_rtattr_t *attr_iptun_ttl =
          nl_rtattr_new(IFLA_IPTUN_TTL, 1, (__u8 *)&link->u.iptun.ttl);
      nl_rtattr_add_child(link_info_data, attr_iptun_ttl);

      nl_rtattr_t *attr_iptun_tos =
          nl_rtattr_new(IFLA_IPTUN_TOS, 1, (__u8 *)&link->u.iptun.tos);
      nl_rtattr_add_child(link_info_data, attr_iptun_tos);

      nl_rtattr_t *attr_iptun_encap_type = nl_rtattr_new(
          IFLA_IPTUN_ENCAP_TYPE, 2, (__u8 *)&link->u.iptun.encap_type);
      nl_rtattr_add_child(link_info_data, attr_iptun_encap_type);

      nl_rtattr_t *attr_iptun_encap_flags = nl_rtattr_new(
          IFLA_IPTUN_ENCAP_FLAGS, 2, (__u8 *)&link->u.iptun.encap_flags);
      nl_rtattr_add_child(link_info_data, attr_iptun_encap_flags);

      __u16 encap_s_port = htons(link->u.iptun.encap_s_port);
      nl_rtattr_t *attr_iptun_encap_s_port =
          nl_rtattr_new(IFLA_IPTUN_ENCAP_SPORT, 2, (__u8 *)&encap_s_port);
      nl_rtattr_add_child(link_info_data, attr_iptun_encap_s_port);

      __u16 encap_d_port = htons(link->u.iptun.encap_d_port);
      nl_rtattr_t *attr_iptun_encap_d_port =
          nl_rtattr_new(IFLA_IPTUN_ENCAP_DPORT, 2, (__u8 *)&encap_d_port);
      nl_rtattr_add_child(link_info_data, attr_iptun_encap_d_port);
    }
  } else if (link->type.ip6tnl) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.ip6tnl.link != 0) {
      nl_rtattr_t *attr_ip6tnl_link =
          nl_rtattr_new(IFLA_IPTUN_LINK, 4, (__u8 *)&link->u.ip6tnl.link);
      nl_rtattr_add_child(link_info_data, attr_ip6tnl_link);
    }

    if (link->u.ip6tnl.local) {
      if (link->u.ip6tnl.local->f.v6) {
        nl_rtattr_t *attr_ip6tnl_local = nl_rtattr_new(
            IFLA_IPTUN_LOCAL, 16, (__u8 *)link->u.ip6tnl.local->v6.bytes);
        nl_rtattr_add_child(link_info_data, attr_ip6tnl_local);
      }
    }
    if (link->u.ip6tnl.remote) {
      if (link->u.ip6tnl.remote->f.v6) {
        nl_rtattr_t *attr_ip6tnl_remote = nl_rtattr_new(
            IFLA_IPTUN_REMOTE, 16, (__u8 *)link->u.ip6tnl.remote->v6.bytes);
        nl_rtattr_add_child(link_info_data, attr_ip6tnl_remote);
      }
    }

    nl_rtattr_t *attr_ip6tnl_ttl =
        nl_rtattr_new(IFLA_IPTUN_TTL, 1, (__u8 *)&link->u.ip6tnl.ttl);
    nl_rtattr_add_child(link_info_data, attr_ip6tnl_ttl);

    nl_rtattr_t *attr_ip6tnl_tos =
        nl_rtattr_new(IFLA_IPTUN_TOS, 1, (__u8 *)&link->u.ip6tnl.tos);
    nl_rtattr_add_child(link_info_data, attr_ip6tnl_tos);

    nl_rtattr_t *attr_ip6tnl_encap_limit = nl_rtattr_new(
        IFLA_IPTUN_ENCAP_LIMIT, 1, (__u8 *)&link->u.ip6tnl.encap_limit);
    nl_rtattr_add_child(link_info_data, attr_ip6tnl_encap_limit);

    nl_rtattr_t *attr_ip6tnl_flags =
        nl_rtattr_new(IFLA_IPTUN_FLAGS, 4, (__u8 *)&link->u.ip6tnl.flags);
    nl_rtattr_add_child(link_info_data, attr_ip6tnl_flags);

    nl_rtattr_t *attr_ip6tnl_proto =
        nl_rtattr_new(IFLA_IPTUN_PROTO, 4, (__u8 *)&link->u.ip6tnl.proto);
    nl_rtattr_add_child(link_info_data, attr_ip6tnl_proto);

    nl_rtattr_t *attr_ip6tnl_flow_info = nl_rtattr_new(
        IFLA_IPTUN_FLOWINFO, 4, (__u8 *)&link->u.ip6tnl.flow_info);
    nl_rtattr_add_child(link_info_data, attr_ip6tnl_flow_info);
  } else if (link->type.sit) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.sittun.link != 0) {
      nl_rtattr_t *attr_sittun_link =
          nl_rtattr_new(IFLA_IPTUN_LINK, 4, (__u8 *)&link->u.sittun.link);
      nl_rtattr_add_child(link_info_data, attr_sittun_link);
    }

    if (link->u.sittun.local) {
      if (link->u.sittun.local->f.v4) {
        nl_rtattr_t *attr_sittun_local = nl_rtattr_new(
            IFLA_IPTUN_LOCAL, 4, (__u8 *)link->u.sittun.local->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_sittun_local);
      }
    }
    if (link->u.sittun.remote) {
      if (link->u.sittun.remote->f.v4) {
        nl_rtattr_t *attr_sittun_remote = nl_rtattr_new(
            IFLA_IPTUN_REMOTE, 4, (__u8 *)link->u.sittun.remote->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_sittun_remote);
      }
    }
    if (link->u.sittun.ttl > 0) {
      nl_rtattr_t *attr_sittun_ttl =
          nl_rtattr_new(IFLA_IPTUN_TTL, 1, (__u8 *)&link->u.sittun.ttl);
      nl_rtattr_add_child(link_info_data, attr_sittun_ttl);
    }

    nl_rtattr_t *attr_sittun_tos =
        nl_rtattr_new(IFLA_IPTUN_TOS, 1, (__u8 *)&link->u.sittun.tos);
    nl_rtattr_add_child(link_info_data, attr_sittun_tos);

    nl_rtattr_t *attr_sittun_p_mtu_disc = nl_rtattr_new(
        IFLA_IPTUN_PMTUDISC, 1, (__u8 *)&link->u.sittun.p_mtu_disc);
    nl_rtattr_add_child(link_info_data, attr_sittun_p_mtu_disc);

    nl_rtattr_t *attr_sittun_encap_type = nl_rtattr_new(
        IFLA_IPTUN_ENCAP_TYPE, 2, (__u8 *)&link->u.sittun.encap_type);
    nl_rtattr_add_child(link_info_data, attr_sittun_encap_type);

    nl_rtattr_t *attr_sittun_encap_flags = nl_rtattr_new(
        IFLA_IPTUN_ENCAP_FLAGS, 2, (__u8 *)&link->u.sittun.encap_flags);
    nl_rtattr_add_child(link_info_data, attr_sittun_encap_flags);

    __u16 encap_s_port = htons(link->u.sittun.encap_s_port);
    nl_rtattr_t *attr_sittun_encap_s_port =
        nl_rtattr_new(IFLA_IPTUN_ENCAP_SPORT, 2, (__u8 *)&encap_s_port);
    nl_rtattr_add_child(link_info_data, attr_sittun_encap_s_port);

    __u16 encap_d_port = htons(link->u.sittun.encap_d_port);
    nl_rtattr_t *attr_sittun_encap_d_port =
        nl_rtattr_new(IFLA_IPTUN_ENCAP_DPORT, 2, (__u8 *)&encap_d_port);
    nl_rtattr_add_child(link_info_data, attr_sittun_encap_d_port);
  } else if (link->type.gre) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.gretun.local) {
      if (link->u.gretun.local->f.v4) {
        nl_rtattr_t *attr_gretun_local = nl_rtattr_new(
            IFLA_GRE_LOCAL, 4, (__u8 *)link->u.gretun.local->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_gretun_local);
      }
    }
    if (link->u.gretun.remote) {
      if (link->u.gretun.remote->f.v4) {
        nl_rtattr_t *attr_gretun_remote = nl_rtattr_new(
            IFLA_GRE_REMOTE, 4, (__u8 *)link->u.gretun.remote->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_gretun_remote);
      }
    }
    if (link->u.gretun.ikey != 0) {
      __u32 ikey = htonl(link->u.gretun.ikey);
      nl_rtattr_t *attr_gretun_ikey =
          nl_rtattr_new(IFLA_GRE_IKEY, 4, (__u8 *)&ikey);
      nl_rtattr_add_child(link_info_data, attr_gretun_ikey);
      link->u.gretun.iflags |= GRE_KEY;
    }
    if (link->u.gretun.okey != 0) {
      __u32 okey = htonl(link->u.gretun.okey);
      nl_rtattr_t *attr_gretun_okey =
          nl_rtattr_new(IFLA_GRE_OKEY, 4, (__u8 *)&okey);
      nl_rtattr_add_child(link_info_data, attr_gretun_okey);
      link->u.gretun.oflags |= GRE_KEY;
    }

    __u16 iflags = htons(link->u.gretun.iflags);
    nl_rtattr_t *attr_gretun_iflags =
        nl_rtattr_new(IFLA_GRE_IFLAGS, 2, (__u8 *)&iflags);
    nl_rtattr_add_child(link_info_data, attr_gretun_iflags);

    __u16 oflags = htons(link->u.gretun.oflags);
    nl_rtattr_t *attr_gretun_oflags =
        nl_rtattr_new(IFLA_GRE_OFLAGS, 2, (__u8 *)&oflags);
    nl_rtattr_add_child(link_info_data, attr_gretun_oflags);

    if (link->u.gretun.link != 0) {
      nl_rtattr_t *attr_gretun_link =
          nl_rtattr_new(IFLA_GRE_LINK, 4, (__u8 *)&link->u.gretun.link);
      nl_rtattr_add_child(link_info_data, attr_gretun_link);
    }

    nl_rtattr_t *attr_gretun_p_mtu_disc =
        nl_rtattr_new(IFLA_GRE_PMTUDISC, 1, (__u8 *)&link->u.gretun.p_mtu_disc);
    nl_rtattr_add_child(link_info_data, attr_gretun_p_mtu_disc);

    nl_rtattr_t *attr_gretun_ttl =
        nl_rtattr_new(IFLA_GRE_TTL, 1, (__u8 *)&link->u.gretun.ttl);
    nl_rtattr_add_child(link_info_data, attr_gretun_ttl);

    nl_rtattr_t *attr_gretun_tos =
        nl_rtattr_new(IFLA_GRE_TOS, 1, (__u8 *)&link->u.gretun.tos);
    nl_rtattr_add_child(link_info_data, attr_gretun_tos);

    nl_rtattr_t *attr_gretun_encap_type = nl_rtattr_new(
        IFLA_GRE_ENCAP_TYPE, 2, (__u8 *)&link->u.gretun.encap_type);
    nl_rtattr_add_child(link_info_data, attr_gretun_encap_type);

    nl_rtattr_t *attr_gretun_encap_flags = nl_rtattr_new(
        IFLA_GRE_ENCAP_FLAGS, 2, (__u8 *)&link->u.gretun.encap_flags);
    nl_rtattr_add_child(link_info_data, attr_gretun_encap_flags);

    __u16 encap_s_port = htons(link->u.gretun.encap_s_port);
    nl_rtattr_t *attr_gretun_encap_s_port =
        nl_rtattr_new(IFLA_GRE_ENCAP_SPORT, 2, (__u8 *)&encap_s_port);
    nl_rtattr_add_child(link_info_data, attr_gretun_encap_s_port);

    __u16 encap_d_port = htons(link->u.gretun.encap_d_port);
    nl_rtattr_t *attr_gretun_encap_d_port =
        nl_rtattr_new(IFLA_GRE_ENCAP_DPORT, 2, (__u8 *)&encap_d_port);
    nl_rtattr_add_child(link_info_data, attr_gretun_encap_d_port);
  } else if (link->type.vti) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    if (link->u.vti.link != 0) {
      nl_rtattr_t *attr_vti_link =
          nl_rtattr_new(IFLA_VTI_LINK, 4, (__u8 *)&link->u.vti.link);
      nl_rtattr_add_child(link_info_data, attr_vti_link);
    }

    if (link->u.vti.local) {
      if (link->u.vti.local->f.v4) {
        nl_rtattr_t *attr_vti_local = nl_rtattr_new(
            IFLA_VTI_LOCAL, 4, (__u8 *)link->u.vti.local->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_vti_local);
      } else if (link->u.vti.local->f.v6) {
        nl_rtattr_t *attr_vti_local = nl_rtattr_new(
            IFLA_VTI_LOCAL, 16, (__u8 *)link->u.vti.local->v6.bytes);
        nl_rtattr_add_child(link_info_data, attr_vti_local);
      }
    }
    if (link->u.vti.remote) {
      if (link->u.vti.remote->f.v4) {
        nl_rtattr_t *attr_vti_remote = nl_rtattr_new(
            IFLA_VTI_REMOTE, 4, (__u8 *)link->u.vti.remote->v4.bytes);
        nl_rtattr_add_child(link_info_data, attr_vti_remote);
      } else if (link->u.vti.remote->f.v6) {
        nl_rtattr_t *attr_vti_remote = nl_rtattr_new(
            IFLA_VTI_REMOTE, 16, (__u8 *)link->u.vti.remote->v6.bytes);
        nl_rtattr_add_child(link_info_data, attr_vti_remote);
      }
    }

    __u32 ikey = htonl(link->u.vti.ikey);
    nl_rtattr_t *attr_vti_ikey = nl_rtattr_new(IFLA_VTI_IKEY, 4, (__u8 *)&ikey);
    nl_rtattr_add_child(link_info_data, attr_vti_ikey);

    __u32 okey = htonl(link->u.vti.okey);
    nl_rtattr_t *attr_vti_okey = nl_rtattr_new(IFLA_VTI_OKEY, 4, (__u8 *)&okey);
    nl_rtattr_add_child(link_info_data, attr_vti_okey);
  } else if (link->type.vrf) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_vrf_table =
        nl_rtattr_new(IFLA_VRF_TABLE, 4, (__u8 *)&link->u.vrf.table);
    nl_rtattr_add_child(link_info_data, attr_vrf_table);
  } else if (link->type.bridge) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);
    if (link->u.bridge.multicast_snooping) {
      nl_rtattr_t *attr_bridge_multicast_snooping = nl_rtattr_new(
          IFLA_BR_MCAST_SNOOPING, 1, (__u8 *)link->u.bridge.multicast_snooping);
      nl_rtattr_add_child(link_info_data, attr_bridge_multicast_snooping);
    }
    if (link->u.bridge.age_time) {
      nl_rtattr_t *attr_bridge_age_time = nl_rtattr_new(
          IFLA_BR_AGEING_TIME, 4, (__u8 *)link->u.bridge.age_time);
      nl_rtattr_add_child(link_info_data, attr_bridge_age_time);
    }
    if (link->u.bridge.hello_time) {
      nl_rtattr_t *attr_bridge_hello_time = nl_rtattr_new(
          IFLA_BR_HELLO_TIME, 4, (__u8 *)link->u.bridge.hello_time);
      nl_rtattr_add_child(link_info_data, attr_bridge_hello_time);
    }
    if (link->u.bridge.vlan_filtering) {
      nl_rtattr_t *attr_bridge_vlan_filtering = nl_rtattr_new(
          IFLA_BR_VLAN_FILTERING, 1, (__u8 *)link->u.bridge.vlan_filtering);
      nl_rtattr_add_child(link_info_data, attr_bridge_vlan_filtering);
    }
  } else if (link->type.gtp) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_gtp_fd0 =
        nl_rtattr_new(IFLA_GTP_FD0, 4, (__u8 *)&link->u.gtp.fd0);
    nl_rtattr_add_child(link_info_data, attr_gtp_fd0);

    nl_rtattr_t *attr_gtp_fd1 =
        nl_rtattr_new(IFLA_GTP_FD1, 4, (__u8 *)&link->u.gtp.fd1);
    nl_rtattr_add_child(link_info_data, attr_gtp_fd1);

    __u32 hashsize = (__u32)131072;
    nl_rtattr_t *attr_gtp_hashsize =
        nl_rtattr_new(IFLA_GTP_PDP_HASHSIZE, 4, (__u8 *)&hashsize);
    nl_rtattr_add_child(link_info_data, attr_gtp_hashsize);

    if (link->u.gtp.role != GTP_ROLE_GGSN) {
      nl_rtattr_t *attr_gtp_role =
          nl_rtattr_new(IFLA_GTP_ROLE, 4, (__u8 *)&link->u.gtp.role);
      nl_rtattr_add_child(link_info_data, attr_gtp_role);
    }
  } else if (link->type.xfrm) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_xfrm_parent_index =
        nl_rtattr_new(IFLA_XFRM_LINK, 4, (__u8 *)&base.parent_index);
    nl_rtattr_add_child(link_info_data, attr_xfrm_parent_index);
    if (link->u.xfrm.ifid != 0) {
      nl_rtattr_t *attr_xfrm_ifid =
          nl_rtattr_new(IFLA_XFRM_IF_ID, 4, (__u8 *)&link->u.xfrm.ifid);
      nl_rtattr_add_child(link_info_data, attr_xfrm_ifid);
    }
  } else if (link->type.ipoib) {
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_ipoib_pkey =
        nl_rtattr_new(IFLA_IPOIB_PKEY, 2, (__u8 *)&link->u.ipoib.pkey);
    nl_rtattr_add_child(link_info_data, attr_ipoib_pkey);

    nl_rtattr_t *attr_ipoib_mode =
        nl_rtattr_new(IFLA_IPOIB_MODE, 2, (__u8 *)&link->u.ipoib.mode);
    nl_rtattr_add_child(link_info_data, attr_ipoib_mode);

    nl_rtattr_t *attr_ipoib_umcast =
        nl_rtattr_new(IFLA_IPOIB_UMCAST, 2, (__u8 *)&link->u.ipoib.umcast);
    nl_rtattr_add_child(link_info_data, attr_ipoib_umcast);
  } else if (link->type.bareudp) {
#define IFLA_BAREUDP_PORT 1
#define IFLA_BAREUDP_ETHERTYPE 2
#define IFLA_BAREUDP_SRCPORT_MIN 3
#define IFLA_BAREUDP_MULTIPROTO_MODE 4
    nl_rtattr_t *link_info_data = nl_rtattr_new(IFLA_INFO_DATA, 0, NULL);
    nl_rtattr_add_child(link_info, link_info_data);

    nl_rtattr_t *attr_bareudp_port =
        nl_rtattr_new(IFLA_BAREUDP_PORT, 2, (__u8 *)&link->u.bareudp.port);
    nl_rtattr_add_child(link_info_data, attr_bareudp_port);

    nl_rtattr_t *attr_bareudp_ether_type = nl_rtattr_new(
        IFLA_BAREUDP_ETHERTYPE, 2, (__u8 *)&link->u.bareudp.ether_type);
    nl_rtattr_add_child(link_info_data, attr_bareudp_ether_type);

    if (link->u.bareudp.src_port_min != 0) {
      nl_rtattr_t *attr_bareudp_src_port_min = nl_rtattr_new(
          IFLA_BAREUDP_SRCPORT_MIN, 2, (__u8 *)&link->u.bareudp.src_port_min);
      nl_rtattr_add_child(link_info_data, attr_bareudp_src_port_min);
    }

    if (link->u.bareudp.multi_proto) {
      nl_rtattr_t *attr_bareudp_multi_proto =
          nl_rtattr_new(IFLA_BAREUDP_MULTIPROTO_MODE, 0, NULL);
      nl_rtattr_add_child(link_info_data, attr_bareudp_multi_proto);
    }
  }

  __u16 attrs_len = 0;
  __u8 *attrs_data = nl_rtattr_serialize(link_info, &attrs_len);
  nl_rtattr_free(link_info);

  ret = nlmsg_append(msg, attrs_data, attrs_len, 0);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
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

bool nl_link_up(int ifi_index) {
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  struct ifinfomsg *nl_req;
  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_NEWLINK,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_ACK);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ifi_family = AF_UNSPEC;
  nl_req->ifi_change = IFF_UP;
  nl_req->ifi_flags = IFF_UP;
  nl_req->ifi_index = ifi_index;
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

bool nl_link_down(int ifi_index) {
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  struct ifinfomsg *nl_req;
  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_NEWLINK,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_ACK);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ifi_family = AF_UNSPEC;
  nl_req->ifi_change = IFF_UP;
  nl_req->ifi_index = ifi_index;
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

bool nl_link_del(int ifi_index) {
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  struct ifinfomsg *nl_req;
  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_DELLINK,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_ACK);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ifi_index = ifi_index;
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

bool nl_link_master(int ifi_index, int master_ifi_index) {
  int ret;
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);
  struct nl_msg *msg = nlmsg_alloc();

  struct ifinfomsg *nl_req;
  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_SETLINK,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_ACK);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ifi_index = ifi_index;

  struct {
    __u16 rta_len;
    __u16 rta_type;
    __u32 rta_val;
  } rta;
  memset(&rta, 0, sizeof(rta));
  rta.rta_type = IFLA_MASTER;
  rta.rta_len = sizeof(rta);
  rta.rta_val = (__u32)master_ifi_index;
  ret = nlmsg_append(msg, &rta, sizeof(rta), RTA_PADDING(rta));
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return false;
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

bool nl_link_no_master(int ifi_index) {
  int master_ifi_index = 0;
  return nl_link_master(ifi_index, master_ifi_index);
}

int _internal_nl_link_addr_list_res(struct nl_msg *msg, void *arg) {
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  if (nlh->nlmsg_type == NLMSG_DONE || nlh->nlmsg_type == NLMSG_ERROR) {
    return NL_SKIP;
  }
  if (nlh->nlmsg_type != RTM_NEWADDR && nlh->nlmsg_type != RTM_DELADDR) {
    return NL_SKIP;
  }

  struct ifaddrmsg *ifa_msg = NLMSG_DATA(nlh);
  struct nl_multi_arg *args = (struct nl_multi_arg *)arg;
  int *addrs_cnt_ptr = (int *)args->arg1;
  nl_addr_mod_t **addrs_ptr = (nl_addr_mod_t **)args->arg2;
  struct ifaddrmsg *nl_req = (struct ifaddrmsg *)args->arg3;

  if (nl_req->ifa_index != 0 && nl_req->ifa_index != ifa_msg->ifa_index) {
    return NL_SKIP;
  }

  if (nl_req->ifa_family != 0 && nl_req->ifa_family != ifa_msg->ifa_family) {
    return NL_SKIP;
  }

  struct rtattr *attrs[IFA_MAX + 1];
  int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa_msg));
  parse_rtattr(attrs, IFA_MAX, IFA_RTA(ifa_msg), remaining);

  __u8 family = ifa_msg->ifa_family;
  nl_addr_mod_t addr;
  memset(&addr, 0, sizeof(addr));
  addr.link_index = ifa_msg->ifa_index;

  ip_net_t local, dst;
  bool has_local = false;
  memset(&local, 0, sizeof(local));
  memset(&dst, 0, sizeof(dst));
  if (attrs[IFA_ADDRESS]) {
    struct rtattr *ifa_addr = attrs[IFA_ADDRESS];
    __u8 *rta_val = (__u8 *)RTA_DATA(ifa_addr);
    if (ifa_addr->rta_len == 8) {
      dst.ip.f.v4 = 1;
      memcpy(dst.ip.v4.bytes, rta_val, 4);
      dst.mask = ifa_msg->ifa_prefixlen;
    } else if (ifa_addr->rta_len == 20) {
      dst.ip.f.v6 = 1;
      memcpy(dst.ip.v6.bytes, rta_val, 16);
      dst.mask = ifa_msg->ifa_prefixlen;
    }
  }
  if (attrs[IFA_LOCAL]) {
    struct rtattr *ifa_addr = attrs[IFA_LOCAL];
    __u8 *rta_val = (__u8 *)RTA_DATA(ifa_addr);
    if (ifa_addr->rta_len == 8) {
      local.ip.f.v4 = 1;
      memcpy(local.ip.v4.bytes, rta_val, 4);
      local.mask = 32;
      has_local = true;
    } else if (ifa_addr->rta_len == 20) {
      local.ip.f.v6 = 1;
      memcpy(local.ip.v6.bytes, rta_val, 16);
      local.mask = 128;
      has_local = true;
    }
  }
  if (attrs[IFA_BROADCAST]) {
    struct rtattr *ifa_addr = attrs[IFA_BROADCAST];
    __u8 *rta_val = (__u8 *)RTA_DATA(ifa_addr);
    if (ifa_addr->rta_len == 8) {
      addr.broadcast.f.v4 = 1;
      memcpy(addr.broadcast.v4.bytes, rta_val, 4);
    } else if (ifa_addr->rta_len == 20) {
      addr.broadcast.f.v6 = 1;
      memcpy(addr.broadcast.v6.bytes, rta_val, 16);
    }
  }
  if (attrs[IFA_LABEL]) {
    struct rtattr *ifa_label = attrs[IFA_LABEL];
    __u8 *rta_val = (__u8 *)RTA_DATA(ifa_label);
    memcpy(addr.label, rta_val, ifa_label->rta_len - 4);
  }
  if (attrs[IFA_FLAGS]) {
    struct rtattr *ifa_flags = attrs[IFA_FLAGS];
    addr.flags = *(__u32 *)RTA_DATA(ifa_flags);
  }
  if (attrs[IFA_CACHEINFO]) {
    struct rtattr *ifa_cache_info = attrs[IFA_CACHEINFO];
    __u32 *cache_info = (__u32 *)RTA_DATA(ifa_cache_info);
    addr.prefered_lft = cache_info[0];
    addr.valid_lft = cache_info[1];
  }

  if (has_local) {
    if (family == FAMILY_V4 && memcmp(&local.ip, &dst.ip, sizeof(ip_t)) == 0) {
      memcpy(&addr.ipnet, &dst, sizeof(ip_net_t));
    } else {
      memcpy(&addr.ipnet, &local, sizeof(ip_net_t));
      memcpy(&addr.peer, &dst, sizeof(ip_net_t));
    }
  } else {
    memcpy(&addr.ipnet, &dst, sizeof(ip_net_t));
  }

  addr.scope = ifa_msg->ifa_scope;

  int addrs_cnt = *addrs_cnt_ptr;
  nl_addr_mod_t *new_addrs = calloc(addrs_cnt + 1, sizeof(addr));
  if (addrs_cnt > 0) {
    memcpy(new_addrs, (nl_addr_mod_t *)*addrs_ptr, addrs_cnt * sizeof(addr));
  }
  memcpy(new_addrs + addrs_cnt, &addr, sizeof(addr));
  if (addrs_cnt > 0) {
    free(*addrs_ptr);
  }

  *addrs_ptr = new_addrs;
  *addrs_cnt_ptr = addrs_cnt + 1;

  return NL_OK;
}

nl_addr_mod_t *nl_link_addr_list(__u32 ifa_index, __u8 family, int *addrs_cnt) {
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);

  struct nl_msg *msg = nlmsg_alloc();

  struct ifaddrmsg *nl_req;

  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_GETADDR,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_DUMP);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ifa_family = family;
  nl_req->ifa_index = ifa_index;

  int ret = nl_send_auto_complete(socket, msg);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return NULL;
  }

  *addrs_cnt = 0;
  nl_addr_mod_t *addrs = NULL;
  struct nl_multi_arg args = {
      .arg1 = addrs_cnt,
      .arg2 = &addrs,
      .arg3 = nl_req,
  };
  nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM,
                      _internal_nl_link_addr_list_res, &args);
  nl_recvmsgs_default(socket);

  nlmsg_free(msg);
  nl_socket_free(socket);

  return addrs;
}