#include <arpa/inet.h>
#include <net/if.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
  memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

  while (RTA_OK(rta, len)) {
    if (rta->rta_type <= max) {
      tb[rta->rta_type] = rta;
    }

    rta = RTA_NEXT(rta, len);
  }
}

static int link_callback(struct nl_msg *msg, void *arg) {
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  struct ifinfomsg *iface = NLMSG_DATA(nlh);
  struct rtattr *hdr = IFLA_RTA(iface);
  int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));

  // printf("Got something.\n");
  // nl_msg_dump(msg, stdout);

  while (RTA_OK(hdr, remaining)) {
    if (hdr->rta_type == IFLA_IFNAME) {
      printf("network interface INDEX: %d", iface->ifi_index);
      printf(" IFNAME: %10s", (char *)RTA_DATA(hdr));
    }
    if (hdr->rta_type == IFLA_LINKINFO) {
      struct rtattr *tb[IFLA_INFO_MAX + 1];
      parse_rtattr(tb, IFLA_INFO_MAX, RTA_DATA(hdr), remaining);
      printf(" IFLA_LINKINFO: %s", (char *)RTA_DATA(tb[IFLA_INFO_KIND]));
    }
    if (hdr->rta_type == IFLA_MTU) {
      __u32 *mtu = RTA_DATA(hdr);
      printf(" MTU: %5d", *mtu);
    }
    if (hdr->rta_type == IFLA_MASTER) {
      __u32 *master = RTA_DATA(hdr);
      printf(" IMASTER: %d", *master);
    }
    if (hdr->rta_type == IFLA_ADDRESS) {
      __u8 *mac = RTA_DATA(hdr);
      printf(" MACADDR: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
    }
    hdr = RTA_NEXT(hdr, remaining);
  }

  printf("\n");

  return NL_OK;
}

static int route_callback(struct nl_msg *msg, void *arg) {
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  struct rtmsg *rtm = NLMSG_DATA(nlh);
  // struct rtattr *hdr = RTM_RTA(rtm);
  struct rtattr *tb[RTA_MAX + 1];
  int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*rtm));

  parse_rtattr(tb, RTA_MAX, RTM_RTA(rtm), remaining);
  // printf("Got something.\n");
  // nl_msg_dump(msg, stdout);

  if (tb[RTA_OIF]) {
    int ifidx = *(__u32 *)RTA_DATA(tb[RTA_OIF]);
    if (ifidx != 2) {
      return NL_OK;
    }
  }

  char buf[256];

  __u32 table = rtm->rtm_table;
  if (tb[RTA_TABLE]) {
    table = *(__u32 *)RTA_DATA(tb[RTA_TABLE]);
  }

  if (rtm->rtm_family != AF_INET && table != RT_TABLE_MAIN) {
    return NL_OK;
  }

  if (tb[RTA_DST]) {
    // if ((rtm->rtm_dst_len != 24) && (rtm->rtm_dst_len != 16)) {
    //     return NL_OK;
    // }

    printf("%s/%u ",
           inet_ntop(rtm->rtm_family, RTA_DATA(tb[RTA_DST]), buf, sizeof(buf)),
           rtm->rtm_dst_len);

  } else if (rtm->rtm_dst_len) {
    printf("0/%u ", rtm->rtm_dst_len);
  } else {
    printf("default ");
  }

  if (tb[RTA_GATEWAY]) {
    printf("via %s", inet_ntop(rtm->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), buf,
                               sizeof(buf)));
  }

  if (tb[RTA_OIF]) {
    char if_nam_buf[IF_NAMESIZE];
    int ifidx = *(__u32 *)RTA_DATA(tb[RTA_OIF]);

    printf(" dev %s", if_indextoname(ifidx, if_nam_buf));
  }

  if (tb[RTA_SRC]) {
    printf("src %s",
           inet_ntop(rtm->rtm_family, RTA_DATA(tb[RTA_SRC]), buf, sizeof(buf)));
  }

  printf("\n");

  return NL_OK;
}

int link_list() {
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);

  struct nl_msg *msg = nlmsg_alloc();

  struct {
    struct ifinfomsg ifh;
    struct {
      __u16 rta_len;
      __u16 rta_type;
      __u32 rta_val;
    } rtattr;
  } * nl_req;

  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_GETLINK,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_DUMP);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->ifh.ifi_family = AF_UNSPEC;
  nl_req->rtattr.rta_type = IFLA_EXT_MASK;
  nl_req->rtattr.rta_len = 8;
  nl_req->rtattr.rta_val = RTEXT_FILTER_VF;

  int ret = nl_send_auto(socket, msg);
  printf("nl_send_auto returned %d\n", ret);

  nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, link_callback, NULL);
  nl_recvmsgs_default(socket);

  return 0;
}

int route_list() {
  struct nl_sock *socket = nl_socket_alloc();
  nl_connect(socket, NETLINK_ROUTE);

  struct nl_msg *msg = nlmsg_alloc();

  struct rtmsg *nl_req;

  struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_GETROUTE,
                                   sizeof(*nl_req), NLM_F_REQUEST | NLM_F_DUMP);

  nl_req = nlmsg_data(nlh);
  memset(nl_req, 0, sizeof(*nl_req));
  nl_req->rtm_family = AF_UNSPEC;

  int ret = nl_send_auto(socket, msg);
  printf("nl_send_auto returned %d\n", ret);

  nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, route_callback, NULL);
  nl_recvmsgs_default(socket);

  return 0;
}

int link_subscribe() {
  struct nl_sock *socket = nl_socket_alloc();
  nl_socket_disable_seq_check(socket);
  nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, link_callback, NULL);
  nl_connect(socket, NETLINK_ROUTE);
  nl_socket_add_memberships(socket, RTNLGRP_LINK, 0);
  while (1) {
    printf("nl_recvmsgs_default\n");
    nl_recvmsgs_default(socket);
  }

  return 0;
}

int main() {
  link_list();
  return 0;
}