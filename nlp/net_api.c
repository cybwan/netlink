#include <net_api.h>
#include <stdio.h>
#include <string.h>

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

int net_addr_add(struct net_api_addr_q *addr) {
  // printf("net_addr_add ");
  // printf("Dev: %-8s ", addr->dev);
  // printf("IP: %-16s", (char *)(addr->ip));
  // printf("\n");
  return 0;
}
int net_addr_del(struct net_api_addr_q *addr) { return 0; }

int net_route_add(struct net_api_route_q *route) {
  // printf("net_route_add ");
  // printf("Protocol: %d ", route->protocol);
  // printf("Flags: %d ", route->flags);
  // printf("Link Index: %2d ", route->link_index);

  // if (route->dst.ip.f.v4) {
  //   struct in_addr *in = (struct in_addr *)route->dst.ip.v4.bytes;
  //   printf("Dst: %s/%d ", inet_ntoa(*in), route->dst.mask);
  // } else if (route->dst.ip.f.v6) {
  //   struct in6_addr *in = (struct in6_addr *)route->dst.ip.v6.bytes;
  //   char a_str[INET6_ADDRSTRLEN];
  //   inet_ntop(AF_INET6, in, a_str, INET6_ADDRSTRLEN);
  //   printf("Dst: %s/%d ", a_str, route->dst.mask);
  // }

  // if (route->gw.f.v4) {
  //   struct in_addr *in = (struct in_addr *)route->gw.v4.bytes;
  //   printf("Gw: %s ", inet_ntoa(*in));
  // } else if (route->gw.f.v6) {
  //   struct in6_addr *in = (struct in6_addr *)route->gw.v6.bytes;
  //   char a_str[INET6_ADDRSTRLEN];
  //   inet_ntop(AF_INET6, in, a_str, INET6_ADDRSTRLEN);
  //   printf("Gw: %s ", a_str);
  // }

  // printf("\n");
  return 0;
}
int net_route_del(struct net_api_route_q *route) { return 0; }

void apply_config_map(const char *name, bool state, bool add) {}