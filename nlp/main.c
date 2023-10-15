#include <net_api.h>
#include <nlp.h>

int main() {
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

  bool loaded = nl_has_loaded_tc_prog("ens33");
  printf("loaded:%d\n", loaded);

  // nl_neigh_list(&port);
  return 0;
}