#include <net_api.h>
#include <nlp.h>

int main() {
  nl_bridge_list();
  nl_link_list();
  // nl_port_mod_t port;
  // nl_link_get(19, &port);
  // debug_link(&port);

  // nl_neigh_list(&port);
  return 0;
}