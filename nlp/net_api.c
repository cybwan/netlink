#include <net_api.h>
#include <stdio.h>

int net_port_add(struct net_api_port_q *port) { return 0; }

int net_port_del(struct net_api_port_q *port) { return 0; }

int net_vlan_add(struct net_api_vlan_q *vlan) {
  printf("net_vlan_add vid:%d mtu:%d name:%s\n", vlan->vid, vlan->mtu,
         vlan->dev);
  return 0;
}

int net_vlan_del(struct net_api_vlan_q *vlan) {
  printf("net_vlan_del vid:%d \n", vlan->vid);
  return 0;
}

void apply_config_map(const char *name, bool state, bool add) {}