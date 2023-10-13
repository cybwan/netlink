package nlp

/*
#include <stdbool.h>
#include <net_api.h>
#include <nlp.h>

extern int net_port_add(struct net_api_port_q *port);
extern int net_port_del(struct net_api_port_q *port);

extern int net_vlan_add(struct net_api_vlan_q *vlan);
extern int net_vlan_del(struct net_api_vlan_q *vlan);

extern int net_vlan_port_add(struct net_api_vlan_port_q *vlan_port);
extern int net_vlan_port_del(struct net_api_vlan_port_q *vlan_port);

extern int net_neigh_add(struct net_api_neigh_q *neigh);
extern int net_neigh_del(struct net_api_neigh_q *neigh);

extern int net_fdb_add(struct net_api_fdb_q *fdb);
extern int net_fdb_del(struct net_api_fdb_q *fdb);

extern int net_addr_add(struct net_api_addr_q *addr);
extern int net_addr_del(struct net_api_addr_q *addr);

extern int net_route_add(struct net_api_route_q *route);
extern int net_route_del(struct net_api_route_q *route);

extern void apply_config_map(char *name, bool state, bool add);


#cgo CFLAGS: -g -Og -W -Wextra -Wno-unused-parameter -I/usr/include/libnl3 -I../../nlp/headers
#cgo LDFLAGS: -L../../nlp -lnl-route-3 -lnl-3 -lev -lnlp
*/
import "C"
import (
	"fmt"
)

//export net_port_add
func net_port_add(port *C.struct_net_api_port_q) C.int {
	fmt.Printf("net_port_add ")
	fmt.Printf("Dev: [%-8s] ", port.dev)
	// fmt.Printf("LinkIndex: %-4d ", port.link_index)
	// fmt.Printf("Ptype: %d ", port.link_type)
	// fmt.Printf("MacAddr: [%3d,%3d,%3d,%3d,%3d,%3d] ", port.mac_addr[0],
	// 	port.mac_addr[1], port.mac_addr[2], port.mac_addr[3],
	// 	port.mac_addr[4], port.mac_addr[5])
	// fmt.Printf("Link: %t ", port.link)
	// fmt.Printf("State: %t ", port.state)
	// fmt.Printf("Mtu: %-5d ", port.mtu)
	// fmt.Printf("Master: %-12s ", port.master)
	// fmt.Printf("Real: %-12s ", port.real)
	// fmt.Printf("TunID: %-4d ", port.tun_id)
	// fmt.Printf("TunSrc: %-50s ", port.tun_src)
	// fmt.Printf("TunDst: %-50s ", port.tun_dst)
	fmt.Printf("\n")
	return C.NL_OK
}

//export net_port_del
func net_port_del(port *C.struct_net_api_port_q) C.int {
	return C.NL_OK
}

//export net_vlan_add
func net_vlan_add(vlan *C.struct_net_api_vlan_q) C.int {
	return C.NL_OK
}

//export net_vlan_del
func net_vlan_del(vlan *C.struct_net_api_vlan_q) C.int {
	return C.NL_OK
}

//export net_vlan_port_add
func net_vlan_port_add(vlan_port *C.struct_net_api_vlan_port_q) C.int {
	return C.NL_OK
}

//export net_vlan_port_del
func net_vlan_port_del(vlan_port *C.struct_net_api_vlan_port_q) C.int {
	return C.NL_OK
}

//export net_neigh_add
func net_neigh_add(neigh *C.struct_net_api_neigh_q) C.int {
	return C.NL_OK
}

//export net_neigh_del
func net_neigh_del(neigh *C.struct_net_api_neigh_q) C.int {
	return C.NL_OK
}

//export net_fdb_add
func net_fdb_add(fdb *C.struct_net_api_fdb_q) C.int {
	return C.NL_OK
}

//export net_fdb_del
func net_fdb_del(fdb *C.struct_net_api_fdb_q) C.int {
	return C.NL_OK
}

//export net_addr_add
func net_addr_add(addr *C.struct_net_api_addr_q) C.int {
	return C.NL_OK
}

//export net_addr_del
func net_addr_del(addr *C.struct_net_api_addr_q) C.int {
	return C.NL_OK
}

//export net_route_add
func net_route_add(route *C.struct_net_api_route_q) C.int {
	return C.NL_OK
}

//export net_route_del
func net_route_del(route *C.struct_net_api_route_q) C.int {
	return C.NL_OK
}

//export apply_config_map
func apply_config_map(name *C.char, state, add C.bool) {
	return
}

func PortList() {
	C.nl_link_list()
}
