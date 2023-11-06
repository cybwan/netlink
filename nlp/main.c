// #include <net_api.h>
// #include <nlp.h>
#include <stdio.h>

#include <lbrt/types.h>

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0;0m\n"

void test_nl_ip_net() {
  char *ip_str = "fe80::20c:29ff:fe79:ab57/64";
  struct nl_ip_net ip_net;
  if (parse_ip_net(ip_str, &ip_net)) {
    printf("ip_net mask=[%d]\n", ip_net.mask);
  }
  printf(GREEN "test_nl_ip_net success\n" RESET);
}

void test_lbrt_counter() {
  struct lbrt_counter *cnt = lbrt_counter_alloc(1, 5);
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  lbrt_counter_put_counter(cnt, 4);
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  printf(YELLOW "counter:[%llu]" RESET, lbrt_counter_get_counter(cnt));
  lbrt_counter_free(cnt);
  printf(GREEN "test_lbrt_counter success\n" RESET);
}

void test_lbrt_layer2() {
  lbrt_l2_h_t *l2h = lbrt_l2_h_alloc(NULL);

  lbrt_fdb_key_t key;
  memset(&key, 0, sizeof(key));
  key.bridge_id = 189;
  key.mac_addr[0] = 0xAA;

  lbrt_fdb_attr_t attr;
  memset(&attr, 0, sizeof(attr));
  attr.type = 8;
  snprintf(attr.oif, IF_NAMESIZE, "ens33");

  lbrt_l2_fdb_add(l2h, &key, &attr);

  lbrt_fdb_ent_t *p, *tmp;
  HASH_ITER(hh, l2h->fdb_map, p, tmp) {
    printf(YELLOW "bridge_id=[%d] type=[%d] oif=[%s]" RESET,
           p->fdb_key.bridge_id, p->fdb_attr.type, p->fdb_attr.oif);
  }

  lbrt_l2_fdb_del(l2h, &key);

  HASH_ITER(hh, l2h->fdb_map, p, tmp) {
    printf(YELLOW "bridge_id=[%d] type=[%d] oif=[%s]" RESET,
           p->fdb_key.bridge_id, p->fdb_attr.type, p->fdb_attr.oif);
  }

  printf(GREEN "test_lbrt_layer2 success" RESET);
}

void test_lbrt_zone() {
  lbrt_zone_h_t *zh = lbrt_zone_h_alloc();

  lbrt_zone_add(zh, "test1");
  lbrt_zone_add(zh, "test2");
  lbrt_zone_add(zh, "test3");
  lbrt_zone_add(zh, "test4");
  lbrt_zone_add(zh, "test5");

  lbrt_zone_t *p, *tmp = NULL;
  HASH_ITER(hh, zh->zone_map, p, tmp) {
    printf(YELLOW "zone_name=[%s] zone_num=[%d]" RESET, p->name, p->zone_num);
  }

  lbrt_zone_t *fp = lbrt_zone_find(zh, "test2");
  if (fp) {
    printf(YELLOW "zone_name=[%s] zone_num=[%d] found success" RESET, fp->name,
           fp->zone_num);
  }
  lbrt_zone_delete(zh, "test2");
  fp = lbrt_zone_find(zh, "test2");
  if (fp) {
    printf(YELLOW "delete fail" RESET);
  } else {
    printf(YELLOW "delete success" RESET);
  }

  int ret = lbrt_zone_br_add(zh, "test4", "test5");
  printf(YELLOW "lbrt_zone_br_add ret=[%d]" RESET, ret);
  lbrt_zone_br_t *br_p, *br_tmp = NULL;
  HASH_ITER(hh, zh->zone_brs, br_p, br_tmp) {
    printf(YELLOW "zone_brs br_name=[%s] zone_name=[%s]" RESET, br_p->br_name,
           br_p->zone->name);
  }
  ret = lbrt_zone_br_delete(zh, "test4");
  printf(YELLOW "lbrt_zone_br_delete ret=[%d]" RESET, ret);
  HASH_ITER(hh, zh->zone_brs, br_p, br_tmp) {
    printf(YELLOW "zone_brs br_name=[%s] zone_name=[%s]" RESET, br_p->br_name,
           br_p->zone->name);
  }

  ret = lbrt_zone_port_add(zh, "test3", "test5");
  printf(YELLOW "lbrt_zone_port_add ret=[%d]" RESET, ret);
  bool valid = lbrt_zone_port_is_valid(zh, "test3", "test5");
  printf(YELLOW "lbrt_zone_port_is_valid valid=[%d]" RESET, valid);
  fp = lbrt_zone_get_by_port(zh, "test3");
  if (fp) {
    printf(YELLOW
           "zone_name=[%s] zone_name=[%s] lbrt_zone_get_by_port success" RESET,
           fp->name, fp->name);
  }

  ret = lbrt_zone_port_delete(zh, "test3");
  printf(YELLOW "lbrt_zone_port_delete ret=[%d]" RESET, ret);
  lbrt_zone_port_t *port_p, *port_tmp = NULL;
  HASH_ITER(hh, zh->zone_ports, port_p, port_tmp) {
    printf(YELLOW "zone_ports port_name=[%s] zone_name=[%s]" RESET,
           port_p->port_name, br_p->zone->name);
  }

  printf(GREEN "test_lbrt_zone success\n" RESET);
}

int main() {
  // test_nl_ip_net();
  test_lbrt_layer2();
  // test_lbrt_counter();
  // test_lbrt_zone();

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

  // int ret = nl_vxlan_bridge_add(1, "ens33");
  // printf("ret=[%d]\n", ret);
  // ret = nl_vxlan_del(1);
  // printf("ret=[%d]\n", ret);

  printf(YELLOW "DONE!" RESET);
  return 0;
}