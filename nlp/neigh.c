#include <net_api.h>
#include <nlp.h>
#include <regex.h>
#include <unistd.h>

#ifndef NDM_RTA
#define NDM_RTA(r)                                                             \
  ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif

#ifndef NDM_PAYLOAD
#define NDM_PAYLOAD(n) NLMSG_PAYLOAD(n, sizeof(struct ndmsg))
#endif

static inline void debug_neigh(nl_neigh_mod_t *neigh) {
  printf("Neigh Master: %2d Index: %2d "
         "MAC: %02x:%02x:%02x:%02x:%02x:%02x "
         "State:%d IP:%x",
         neigh->master_index, neigh->link_index, neigh->hwaddr[0],
         neigh->hwaddr[1], neigh->hwaddr[2], neigh->hwaddr[3], neigh->hwaddr[4],
         neigh->hwaddr[5], neigh->state, neigh->ip);
  printf("\n");
}

int nl_neigh_add(nl_neigh_mod_t *neigh, nl_port_mod_t *port) {
  if (is_zero_mac(neigh->hwaddr)) {
    return -1;
  }
  if (neigh->family == AF_INET || neigh->family == AF_INET6) {
    struct net_api_neigh_q neigh_q;
    neigh_q.ip = neigh->ip;
    neigh_q.link_index = neigh->link_index;
    neigh_q.state = neigh->state;
    memcpy(neigh_q.hwaddr, neigh->hwaddr, 6);
    net_neigh_add(&neigh_q);
  } else if (neigh->family == AF_BRIDGE) {
    if (neigh->vlan == 1) {
      /*FDB comes with vlan 1 also */
      return 0;
    }
    if ((neigh->hwaddr[0] & 0x01) == 1 || neigh->hwaddr[0] == 0) {
      /* Multicast MAC or ZERO address --- IGNORED */
      return 0;
    }

    int brId = 0;
    int ftype;
    int dst;

    if (neigh->master_index > 0) {
      nl_port_mod_t brLink;
      if (nl_link_get(neigh->master_index, &brLink) < 0) {
        return -1;
      }
      if (memcmp(brLink.hwaddr, neigh->hwaddr, 6) == 0) {
        /*Same as bridge mac --- IGNORED */
        return 0;
      }

      regex_t regex;
      const size_t nmatch = 1;
      regmatch_t pmatch[1];
      regcomp(&regex, "[0-9]+", REG_EXTENDED);
      int status = regexec(&regex, (char *)brLink.name, nmatch, pmatch, 0);
      if (status == 0) {
        char str_buf[IF_NAMESIZE];
        strncpy(str_buf, (char *)port->name + pmatch[0].rm_so,
                pmatch[0].rm_eo - pmatch[0].rm_so);
        brId = atoi(str_buf);
      }
      regfree(&regex);
    }

    if (port->type.vxlan) {
      /* Interested in only VxLAN FDB */
      if (neigh->ip > 0 && neigh->master_index == 0) {
        dst = neigh->ip;
        brId = port->u.vxlan.vxlan_id;
        ftype = FDB_TUN;
      } else {
        return 0;
      }
    } else {
      dst = 0;
      ftype = FDB_VLAN;
    }

    struct net_api_fdb_q fdb_q;
    fdb_q.bridge_id = brId;
    fdb_q.type = ftype;
    memcpy(fdb_q.mac_addr, neigh->hwaddr, 6);
    memcpy(fdb_q.dev, port->name, IF_NAMESIZE);
    struct in_addr *in = (struct in_addr *)&dst;
    inet_ntop(AF_INET, in, (char *)fdb_q.dst, INET_ADDRSTRLEN);
    return net_fdb_add(&fdb_q);
  }
  return 0;
}

int nl_neigh_list_res(struct nl_msg *msg, void *arg) {
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  struct ndmsg *neigh_msg = NLMSG_DATA(nlh);
  struct nl_multi_arg *args = (struct nl_multi_arg *)arg;
  struct nl_port_mod *port = (struct nl_port_mod *)args->arg1;
  struct ndmsg *nl_req = (struct ndmsg *)args->arg2;

  if (nl_req->ndm_ifindex != 0 &&
      nl_req->ndm_ifindex != neigh_msg->ndm_ifindex) {
    return NL_SKIP;
  }

  if (nl_req->ndm_family != 0 && nl_req->ndm_family != neigh_msg->ndm_family) {
    return NL_SKIP;
  }

  if (nl_req->ndm_state != 0 && nl_req->ndm_state != neigh_msg->ndm_state) {
    return NL_SKIP;
  }

  if (nl_req->ndm_type != 0 && nl_req->ndm_type != neigh_msg->ndm_type) {
    return NL_SKIP;
  }

  if (nl_req->ndm_flags != 0 && nl_req->ndm_flags != neigh_msg->ndm_flags) {
    return NL_SKIP;
  }

  struct rtattr *attrs[NDA_MAX + 1];
  int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*neigh_msg));
  parse_rtattr(attrs, NDA_MAX, NDM_RTA(neigh_msg), remaining);

  nl_neigh_mod_t neigh;
  memset(&neigh, 0, sizeof(neigh));
  neigh.link_index = neigh_msg->ndm_ifindex;
  neigh.family = neigh_msg->ndm_family;
  neigh.state = neigh_msg->ndm_state;
  neigh.type = neigh_msg->ndm_type;
  neigh.flags = neigh_msg->ndm_flags;

  if (attrs[NDA_MASTER]) {
    neigh.master_index = *(__u32 *)RTA_DATA(attrs[NDA_MASTER]);
  }

  if (attrs[NDA_VNI]) {
    neigh.vni = *(__u32 *)RTA_DATA(attrs[NDA_VNI]);
  }

  if (attrs[NDA_VLAN]) {
    neigh.vlan = *(__u32 *)RTA_DATA(attrs[NDA_VLAN]);
  }

  if (attrs[NDA_DST]) {
    neigh.ip = *(__u32 *)RTA_DATA(attrs[NDA_DST]);
  }

  if (attrs[NDA_LLADDR]) {
    struct rtattr *lladdr = attrs[NDA_LLADDR];
    int attr_len = RTA_PAYLOAD(lladdr);
    if (attr_len == 4) {
      neigh.ll_ip_addr = *(__u32 *)RTA_DATA(lladdr);

    } else if (attr_len == 16) {
      // Can be IPv6 or FireWire HWAddr

      // FIXME ? EncapType? tunnel6
      // nl_port_mod_t link;
      // memset(&link, 0, sizeof(nl_port_mod_t));
      // int ret = nl_link_get(neigh.link_index, &link);

      __u8 *hwaddr = (__u8 *)RTA_DATA(lladdr);
      memcpy(neigh.hwaddr, hwaddr, attr_len);
    } else {
      __u8 *hwaddr = (__u8 *)RTA_DATA(lladdr);
      memcpy(neigh.hwaddr, hwaddr, attr_len);
    }
  }

  // debug_neigh(&neigh);
  nl_neigh_add(&neigh, port);

  return NL_OK;
}

int nl_neigh_list(nl_port_mod_t *port, __u8 family) {
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);

  struct nl_msg *msg = nlmsg_alloc();

  struct ndmsg *nl_req;

  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_GETNEIGH,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_DUMP);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ndm_family = family;
  nl_req->ndm_ifindex = port->index;

  int ret = nl_send_auto_complete(socket, msg);
  if (ret < 0) {
    nlmsg_free(msg);
    nl_socket_free(socket);
    return ret;
  }

  struct nl_multi_arg args = {
      .arg1 = port,
      .arg2 = nl_req,
  };
  nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, nl_neigh_list_res,
                      &args);
  nl_recvmsgs_default(socket);

  nlmsg_free(msg);
  nl_socket_free(socket);

  return 0;
}