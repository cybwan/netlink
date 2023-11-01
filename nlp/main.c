#include <net_api.h>
#include <nlp.h>
#include <stdio.h>
#include <stdlib.h>

static inline void debug_addr1(nl_addr_mod_t *addr) {
  printf("Addr Link Index: %2d ", addr->link_index);

  if (addr->peer.ip.f.v4) {
    struct in_addr *in = (struct in_addr *)addr->peer.ip.v4.bytes;
    printf("peer: %s/%d ", inet_ntoa(*in), addr->peer.mask);
  } else if (addr->peer.ip.f.v6) {
    struct in6_addr *in = (struct in6_addr *)addr->peer.ip.v6.bytes;
    char a_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, in, a_str, INET6_ADDRSTRLEN);
    printf("peer: %s/%d ", a_str, addr->peer.mask);
  }

  if (addr->ipnet.ip.f.v4) {
    struct in_addr *in = (struct in_addr *)addr->ipnet.ip.v4.bytes;
    printf("ipnet: %12s/%02d ", inet_ntoa(*in), addr->ipnet.mask);
  } else if (addr->ipnet.ip.f.v6) {
    struct in6_addr *in = (struct in6_addr *)addr->ipnet.ip.v6.bytes;
    char a_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, in, a_str, INET6_ADDRSTRLEN);
    printf("ipnet: %12s/%d ", a_str, addr->ipnet.mask);
  }

  if (addr->broadcast.f.v4) {
    struct in_addr *in = (struct in_addr *)addr->broadcast.v4.bytes;
    printf("broadcast: %s ", inet_ntoa(*in));
  } else if (addr->broadcast.f.v6) {
    struct in6_addr *in = (struct in6_addr *)addr->broadcast.v6.bytes;
    char a_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, in, a_str, INET6_ADDRSTRLEN);
    printf("broadcast: %s ", a_str);
  }

  printf("scope: %3d ", addr->scope);
  printf("flags: %3d ", addr->flags);
  printf("label: %5s ", addr->label);
  printf("prefered_lft: %u ", addr->prefered_lft);
  printf("valid_lft: %u ", addr->valid_lft);

  printf("\n");
}

int main() {
  // nl_debug = 0;
  // nl_rtattr_t *info = nl_rtattr_new(1, 0, NULL);

  // nl_rtattr_t *info_kind = nl_rtattr_alloc();
  // info_kind->rta_type = 2;

  // nl_rtattr_t *info_data = nl_rtattr_alloc();
  // info_data->rta_type = 3;

  // nl_rtattr_add_child(info, info_data);
  // nl_rtattr_add_child(info, info_kind);

  // nl_rtattr_t *tm;

  // nl_list_for_each_entry(tm, &info->children, nl_list) {
  //   printf("%d\n", tm->rta_type);
  // }

  // printf("info len=[%d]\n", nl_rtattr_len(info));

  // __u16 len = 0;
  // __u8 *data = nl_rtattr_serialize(info, &len);
  // for (int i = 0; i < len; i++) {
  //   printf("%d ", data[i]);
  // }
  // printf("\n");

  // // nl_rtattr_free(info_kind);
  // nl_rtattr_free(info);
  // // nl_rtattr_free(info_data);
  // // nl_rtattr_free_child(&info->children);

  // nl_bridge_list();
  // nl_link_list();
  // nl_link_subscribe();

  // nl_addr_subscribe();
  // nl_neigh_subscribe();
  // nl_route_subscribe();

  // nl_port_mod_t port;
  // nl_link_get_by_index(3, &port);
  // printf("port.name:%s\n",port.name);

  // int ret = nl_link_get_by_name("ens33", &port);
  // printf("port.index:%d  name:%s ret=%d\n", port.index, port.name, ret);

  // bool loaded = nl_has_loaded_tc_prog("ens33");
  // printf("loaded:%d\n", loaded);

  // nl_neigh_list(&port);

  // char *ip_str = "fe80::20c:29ff:fe79:ab57/64";

  // struct nl_ip_net ip_net;
  // if (parse_ip_net(ip_str, &ip_net)) {
  //   printf("ip_net mask=[%d]\n", ip_net.mask);
  // }

  // bool ret = nl_route_add("7.7.7.0/24", "192.168.127.1");
  // printf("success:[%d]\n", ret);

  // bool ret = nl_route_del("7.7.7.0/24");
  // printf("success:[%d]\n", ret);

  // bool ret = nl_addr_add("ee00::/64 ens33:1", "ens33");
  // printf("add success:[%d]\n", ret);
  // ret = nl_addr_del("ee00::/64 ens33:1", "ens33");
  // printf("del success:[%d]\n", ret);
  // ret = nl_addr_add("7.7.7.0/24 ens33:1", "ens33");
  // printf("add success:[%d]\n", ret);
  // ret = nl_addr_del("7.7.7.0/24 ens33:1", "ens33");
  // printf("del success:[%d]\n", ret);

  // bool ret = nl_fdb_add("11:11:11:11:11:11", "ens33");
  // printf("add success:[%d]\n", ret);

  // bool ret = nl_fdb_del("11:11:11:11:11:11", "ens33");
  // printf("del success:[%d]\n", ret);

  // bool ret = nl_neigh_add("192.168.118.1", "ens33", "11:11:11:11:11:11");
  // printf("add success:[%d]\n", ret);

  // bool ret = nl_neigh_del("192.168.118.1", "ens33");
  // printf("add success:[%d]\n", ret);

  // nl_link_t link;
  // memset(&link, 0, sizeof(link));
  // link.attrs.name = "testa";
  // link.attrs.mtu = 9000;
  // link.type.bridge = 1;
  // if (nl_link_add(&link, NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK)) {
  //   printf("success\n");
  // }

  // nl_vlan_add(1);
  // nl_vlan_member_add(1, "ens36", true);
  // nl_vlan_member_del(1, "ens36", true);
  // nl_vlan_del(1);

  // int vlan_id = 1;
  // char vlan_dev_name[IF_NAMESIZE];
  // memset(vlan_dev_name, 0, IF_NAMESIZE);
  // snprintf(vlan_dev_name, IF_NAMESIZE, "%s.%d", "demo", vlan_id);
  // // snprintf(vlan_dev_name, IF_NAMESIZE, "%s", "demo");
  // nl_link_t vlan_link;
  // memset(&vlan_link, 0, sizeof(vlan_link));
  // vlan_link.attrs.name = vlan_dev_name;
  // vlan_link.attrs.parent_index = (__s32)3;
  // // vlan_link.type.bridge = 1;
  // // vlan_link.type.veth = 1;
  // // vlan_link.attrs.mtu = 1340;
  // // vlan_link.u.veth.peer_name = "veth";
  // vlan_link.type.vlan = 1;
  // vlan_link.u.vlan.vlan_id = 2;
  // if (!nl_link_add(&vlan_link, NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK)) {
  //   printf("failed\n");
  //   return 0;
  // }

  // int addrs_cnt = 0;
  // nl_addr_mod_t *addrs = nl_link_addr_list(1, FAMILY_V4, &addrs_cnt);
  // for (int i = 0; i < addrs_cnt; i++) {
  //   debug_addr1(&addrs[i]);
  // }
  // printf("addrs_cnt=[%d]\n", addrs_cnt);

  int ret = nl_vxlan_bridge_add(1, "ens33");
  printf("ret=[%d]\n", ret);
  ret = nl_vxlan_del(1);
  printf("ret=[%d]\n", ret);
  printf("success\n");

  return 0;
}