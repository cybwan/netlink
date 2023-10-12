#include <net_api.h>
#include <stdio.h>

int net_port_add(struct net_api_port_q *port) { return 0; }
int net_port_del(struct net_api_port_q *port) { return 0; }
int net_vlan_add(struct net_api_vlan_q *vlan) { return 0; }
int net_vlan_del(struct net_api_vlan_q *vlan) { return 0; }
int net_vlan_port_add(struct net_api_vlan_port_q *vlan_port) { return 0; }
int net_vlan_port_del(struct net_api_vlan_port_q *vlan_port) { return 0; }
int net_neigh_add(struct net_api_neigh_q *neigh) { return 0; }
int net_neigh_del(struct net_api_neigh_q *neigh) { return 0; }
int net_fdb_add(struct net_api_fdb_q *fdb) {
  printf("net_fdb_add dst: %d via dev: %s mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
         fdb->dst, fdb->dev, fdb->mac_addr[0], fdb->mac_addr[1],
         fdb->mac_addr[2], fdb->mac_addr[3], fdb->mac_addr[4],
         fdb->mac_addr[5]);
  return 0;
}
int net_fdb_del(struct net_api_fdb_q *fdb) { return 0; }

void apply_config_map(const char *name, bool state, bool add) {}