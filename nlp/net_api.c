#include <net_api.h>
#include <stdio.h>
#include <string.h>

int net_port_add(struct net_api_port_q *port) {
  printf("net_port_add ");
  printf("Dev: %-8s ", (char *)(port->dev));
  printf("LinkIndex: %-4d ", port->link_index);
  printf("Ptype: %d ", port->link_type);

  printf("MacAddr: [%d,%d,%d,%d,%d,%d] ", port->mac_addr[0], port->mac_addr[1],
         port->mac_addr[2], port->mac_addr[3], port->mac_addr[4],
         port->mac_addr[5]);
  printf("Link: %d ", port->link);
  printf("State: %d ", port->state);
  printf("Mtu: %-4d ", port->mtu);
  printf("Master: %s ", (char *)port->master);
  printf("Real: %s ", (char *)port->real);
  printf("Mtu: %d ", port->tun_id);

  struct in_addr *tun_src_in = (struct in_addr *)&port->tun_src;
  printf("TunSrc: %s ", inet_ntoa(*tun_src_in));

  struct in_addr *tun_dst_in = (struct in_addr *)&port->tun_dst;
  printf("TunDst: %s ", inet_ntoa(*tun_dst_in));

  printf("\n");
  return 0;
}
int net_port_del(struct net_api_port_q *port) { return 0; }

int net_vlan_add(struct net_api_vlan_q *vlan) {
  printf("net_vlan_add ");
  printf("vid: %-4d ", vlan->vid);
  printf("dev: %-8s ", (char *)(vlan->dev));
  printf("LinkIndex: %-4d ", vlan->link_index);
  printf("MacAddr: [%d,%d,%d,%d,%d,%d] ", vlan->mac_addr[0], vlan->mac_addr[1],
         vlan->mac_addr[2], vlan->mac_addr[3], vlan->mac_addr[4],
         vlan->mac_addr[5]);
  printf("Link: %d ", vlan->link);
  printf("State: %d ", vlan->state);
  printf("Mtu: %-4d ", vlan->mtu);
  printf("TunID: %-4d ", vlan->tun_id);
  printf("\n");
  return 0;
}

int net_vlan_del(struct net_api_vlan_q *vlan) { return 0; }

int net_vlan_port_add(struct net_api_vlan_port_q *vlan_port) {
  printf("net_vlan_port_add ");
  printf("vid: %-4d ", vlan_port->vid);
  printf("dev: %-8s ", (char *)(vlan_port->dev));
  printf("tagged: %d ", vlan_port->tagged);
  printf("\n");
  return 0;
}

int net_vlan_port_del(struct net_api_vlan_port_q *vlan_port) { return 0; }

int net_neigh_add(struct net_api_neigh_q *neigh) {
  // printf("net_neigh_add ");

  // struct in_addr *in = (struct in_addr *)&neigh->ip;
  // printf("IP: %s ", inet_ntoa(*in));

  // printf("LinkIndex: %-4d ", neigh->link_index);
  // printf("State: %d ", neigh->state);
  // printf("HardwareAddr: [%d,%d,%d,%d,%d,%d] ", neigh->hwaddr[0],
  //        neigh->hwaddr[1], neigh->hwaddr[2], neigh->hwaddr[3], neigh->hwaddr[4],
  //        neigh->hwaddr[5]);

  // printf("\n");
  return 0;
}

int net_neigh_del(struct net_api_neigh_q *neigh) { return 0; }

int net_fdb_add(struct net_api_fdb_q *fdb) {
  printf("net_fdb_add ");
  printf("MacAddr: [%d,%d,%d,%d,%d,%d] ", fdb->mac_addr[0], fdb->mac_addr[1],
         fdb->mac_addr[2], fdb->mac_addr[3], fdb->mac_addr[4],
         fdb->mac_addr[5]);
  printf("BridgeID: %d ", fdb->bridge_id);
  printf("Dev: %-8s ", (char *)(fdb->dev));

  struct in_addr *in = (struct in_addr *)&fdb->dst;
  printf("Dst: %s ", inet_ntoa(*in));

  printf("Type: %d ", fdb->type);
  printf("\n");
  return 0;
}

int net_fdb_del(struct net_api_fdb_q *fdb) { return 0; }

int net_addr_add(struct net_api_addr_q *addr) {
  printf("net_addr_add ");
  printf("Dev: %-8s ", addr->dev);
  printf("IP: %-16s", (char *)(addr->ip));
  printf("\n");
  return 0;
}

int net_addr_del(struct net_api_addr_q *addr) { return 0; }

int net_route_add(struct net_api_route_q *route) {
  printf("net_route_add ");
  printf("Protocol: %d ", route->protocol);
  printf("Flags: %d ", route->flags);
  printf("Link Index: %2d ", route->link_index);

  if (route->dst.ip.f.v4) {
    struct in_addr *in = (struct in_addr *)route->dst.ip.v4.bytes;
    printf("Dst: %s/%d ", inet_ntoa(*in), route->dst.mask);
  } else if (route->dst.ip.f.v6) {
    struct in6_addr *in = (struct in6_addr *)route->dst.ip.v6.bytes;
    char a_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, in, a_str, INET6_ADDRSTRLEN);
    printf("Dst: %s/%d ", a_str, route->dst.mask);
  }

  if (route->gw.f.v4) {
    struct in_addr *in = (struct in_addr *)route->gw.v4.bytes;
    printf("Gw: %s ", inet_ntoa(*in));
  } else if (route->gw.f.v6) {
    struct in6_addr *in = (struct in6_addr *)route->gw.v6.bytes;
    char a_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, in, a_str, INET6_ADDRSTRLEN);
    printf("Gw: %s ", a_str);
  }

  printf("\n");
  return 0;
}

int net_route_del(struct net_api_route_q *route) { return 0; }

void apply_config_map(const char *name, bool state, bool add) {}