#include <net_api.h>
#include <stdio.h>
#include <string.h>

struct {
  __u32 net_port_add : 1;
  __u32 net_port_del : 1;
  __u32 net_vlan_add : 1;
  __u32 net_vlan_del : 1;
  __u32 net_vlan_port_add : 1;
  __u32 net_vlan_port_del : 1;
  __u32 net_neigh_add : 1;
  __u32 net_neigh_del : 1;
  __u32 net_fdb_add : 1;
  __u32 net_fdb_del : 1;
  __u32 net_addr_add : 1;
  __u32 net_addr_del : 1;
  __u32 net_route_add : 1;
  __u32 net_route_del : 1;
} debug = {
    .net_port_add = 0,
    .net_port_del = 0,
    .net_vlan_add = 0,
    .net_vlan_del = 0,
    .net_vlan_port_add = 0,
    .net_vlan_port_del = 0,
    .net_neigh_add = 0,
    .net_neigh_del = 0,
    .net_fdb_add = 0,
    .net_fdb_del = 0,
    .net_addr_add = 0,
    .net_addr_del = 0,
    .net_route_add = 1,
    .net_route_del = 1,
};

int net_port_add(struct net_api_port_q *port) {
  if (debug.net_port_add) {
    printf("net_port_add ");
    printf("Dev: %-8s ", (char *)(port->dev));
    printf("LinkIndex: %-4d ", port->link_index);
    printf("Ptype: %d ", port->link_type);
    printf("MacAddr: [%3d,%3d,%3d,%3d,%3d,%3d] ", port->mac_addr[0],
           port->mac_addr[1], port->mac_addr[2], port->mac_addr[3],
           port->mac_addr[4], port->mac_addr[5]);
    printf("Link: %d ", port->link);
    printf("State: %d ", port->state);
    printf("Mtu: %-5d ", port->mtu);
    printf("Master: %-12s ", (char *)port->master);
    printf("Real: %-12s ", (char *)port->real);
    printf("TunID: %-4d ", port->tun_id);
    printf("TunSrc: %-50s ", port->tun_src);
    printf("TunDst: %-50s ", port->tun_dst);
    printf("\n");
  }
  return 0;
}
int net_port_del(struct net_api_port_q *port) {
  if (debug.net_port_del) {
    printf("net_port_del ");
    printf("Dev: %-8s ", (char *)(port->dev));
    printf("Ptype: %2d ", port->link_type);
    printf("\n");
  }
  return 0;
}

int net_vlan_add(struct net_api_vlan_q *vlan) {
  if (debug.net_vlan_add) {
    printf("net_vlan_add ");
    printf("vid: %-4d ", vlan->vid);
    printf("dev: %-8s ", (char *)(vlan->dev));
    printf("LinkIndex: %-4d ", vlan->link_index);
    printf("MacAddr: [%3d,%3d,%3d,%3d,%3d,%3d] ", vlan->mac_addr[0],
           vlan->mac_addr[1], vlan->mac_addr[2], vlan->mac_addr[3],
           vlan->mac_addr[4], vlan->mac_addr[5]);
    printf("Link: %2d ", vlan->link);
    printf("State: %2d ", vlan->state);
    printf("Mtu: %-4d ", vlan->mtu);
    printf("TunID: %-4d ", vlan->tun_id);
    printf("\n");
  }
  return 0;
}

int net_vlan_del(struct net_api_vlan_q *vlan) {
  if (debug.net_vlan_del) {
    printf("net_vlan_del ");
    printf("vid: %-4d ", vlan->vid);
    printf("\n");
  }
  return 0;
}

int net_vlan_port_add(struct net_api_vlan_port_q *vlan_port) {
  if (debug.net_vlan_port_add) {
    printf("net_vlan_port_add ");
    printf("vid: %-4d ", vlan_port->vid);
    printf("dev: %-8s ", (char *)(vlan_port->dev));
    printf("tagged: %d ", vlan_port->tagged);
    printf("\n");
  }
  return 0;
}

int net_vlan_port_del(struct net_api_vlan_port_q *vlan_port) {
  if (debug.net_vlan_port_del) {
    printf("net_vlan_port_del ");
    printf("vid: %-4d ", vlan_port->vid);
    printf("dev: %-8s ", (char *)(vlan_port->dev));
    printf("tagged: %d ", vlan_port->tagged);
    printf("\n");
  }
  return 0;
}

int net_neigh_add(struct net_api_neigh_q *neigh) {
  if (debug.net_neigh_add) {
    printf("net_neigh_add ");
    printf("IP: %-50s ", neigh->ip);
    printf("LinkIndex: %-4d ", neigh->link_index);
    printf("State: %2d ", neigh->state);
    printf("HardwareAddr: [%3d,%3d,%3d,%3d,%3d,%3d] ", neigh->hwaddr[0],
           neigh->hwaddr[1], neigh->hwaddr[2], neigh->hwaddr[3],
           neigh->hwaddr[4], neigh->hwaddr[5]);
    printf("\n");
  }
  return 0;
}

int net_neigh_del(struct net_api_neigh_q *neigh) {
  if (debug.net_neigh_del) {
    printf("net_neigh_del ");
    printf("IP: %-50s ", neigh->ip);
    printf("\n");
  }
  return 0;
}

int net_fdb_add(struct net_api_fdb_q *fdb) {
  if (debug.net_fdb_add) {
    printf("net_fdb_add ");
    printf("MacAddr: [%3d,%3d,%3d,%3d,%3d,%3d] ", fdb->mac_addr[0],
           fdb->mac_addr[1], fdb->mac_addr[2], fdb->mac_addr[3],
           fdb->mac_addr[4], fdb->mac_addr[5]);
    printf("BridgeID: %d ", fdb->bridge_id);
    printf("Dev: %-8s ", (char *)(fdb->dev));
    printf("Dst: %-50s ", (char *)(fdb->dst));
    printf("Type: %d ", fdb->type);
    printf("\n");
  }
  return 0;
}

int net_fdb_del(struct net_api_fdb_q *fdb) {
  if (debug.net_fdb_del) {
    printf("net_fdb_del ");
    printf("MacAddr: [%3d,%3d,%3d,%3d,%3d,%3d] ", fdb->mac_addr[0],
           fdb->mac_addr[1], fdb->mac_addr[2], fdb->mac_addr[3],
           fdb->mac_addr[4], fdb->mac_addr[5]);
    printf("BridgeID: %d ", fdb->bridge_id);
    printf("\n");
  }
  return 0;
}

int net_addr_add(struct net_api_addr_q *addr) {
  if (debug.net_addr_add) {
    printf("net_addr_add ");
    printf("Dev: %-8s ", addr->dev);
    printf("IP: %-50s", (char *)(addr->ip));
    printf("\n");
  }
  return 0;
}

int net_addr_del(struct net_api_addr_q *addr) {
  if (debug.net_addr_del) {
    printf("net_addr_del ");
    printf("Dev: %-8s ", addr->dev);
    printf("IP: %-50s", (char *)(addr->ip));
    printf("\n");
  }
  return 0;
}

int net_route_add(struct net_api_route_q *route) {
  if (debug.net_route_add) {
    printf("net_route_add ");
    printf("Protocol: %2d ", route->protocol);
    printf("Flags: %2d ", route->flags);
    printf("Link Index: %2d ", route->link_index);
    printf("Dst: %-50s ", route->dst);
    printf("Gw: %-50s ", route->gw);
    printf("\n");
  }
  return 0;
}

int net_route_del(struct net_api_route_q *route) {
  if (debug.net_route_del) {
    printf("net_route_del ");
    printf("Dst: %-50s ", route->dst);
    printf("\n");
  }
  return 0;
}

void apply_config_map(char *name, bool state, bool add) {}