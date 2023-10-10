package nlp

/*
#include <linux/types.h>

#include <net/if.h>
#include <arpa/inet.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include <stdio.h>

typedef struct nl_port_mod {
  __u8  dev[IF_NAMESIZE];
  __u32 index;
  __u32 type;
  __u8  mac_addr;
  __u8  link_state;
  __u32 mtu;
  __u8  master[IF_NAMESIZE];
  __u8  real[IF_NAMESIZE];
  __u32 tun_id;
  __u32 tun_src[4];
  __u32 tun_dst[4];
} nl_port_mod_t;

typedef struct nl_vlan_mod {
} nl_vlan_mod_t;

typedef struct nl_vlan_port_mod {
} nl_vlan_port_mod_t;

typedef struct nl_ip_addr_mod {
} nl_ip_addr_mod_t;

typedef struct nl_neigh_mod {
} nl_neigh_mod_t;

typedef struct nl_route_mod {
} nl_route_mod_t;

typedef struct nl_fdb_mod {
} nl_fdb_mod_t;

static int port_list();
static int port_parse();
extern int net_port_add(nl_port_mod_t *port);

static int port_parse(struct nl_msg *msg, void *arg) {
	nl_port_mod_t port;
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct ifinfomsg *iface = NLMSG_DATA(nlh);
    struct rtattr *hdr = IFLA_RTA(iface);
    int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));

    while (RTA_OK(hdr, remaining)) {
        if (hdr->rta_type == IFLA_IFNAME) {
            printf("network interface INDEX: %d", iface->ifi_index);
            printf(" IFNAME: %10s", (char *) RTA_DATA(hdr));
        }
        if (hdr->rta_type == IFLA_LINKINFO) {
            printf(" IFLA_LINKINFO: %s", (char *) RTA_DATA(hdr));
        }
        if (hdr->rta_type == IFLA_MTU) {
            __u32 *mtu = RTA_DATA(hdr);
			port.mtu = *mtu;
            printf(" MTU: %5d", *mtu);
        }
        if (hdr->rta_type == IFLA_MASTER) {
            __u32 *master = RTA_DATA(hdr);
            printf(" IMASTER: %d", *master);
        }
        if (hdr->rta_type == IFLA_ADDRESS) {
            __u8 *mac = RTA_DATA(hdr);
            printf(" MACADDR: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }

        hdr = RTA_NEXT(hdr, remaining);
    }

    printf("\n");
	net_port_add(&port);

    return NL_OK;
}

static int port_list() {
    struct nl_sock *socket = nl_socket_alloc();
    nl_connect(socket, NETLINK_ROUTE);

    struct nl_msg *msg = nlmsg_alloc();

    struct {
        struct ifinfomsg ifh;
        struct  {
            __u16 rta_len;
            __u16 rta_type;
            __u32 rta_val;
        } rtattr;
    } *nl_req;

    struct nlmsghdr *nlh = nlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, RTM_GETLINK, sizeof(*nl_req), NLM_F_REQUEST | NLM_F_DUMP);

    nl_req = nlmsg_data(nlh);
	memset(nl_req, 0, sizeof(*nl_req));
    nl_req->ifh.ifi_family = AF_UNSPEC;
    nl_req->rtattr.rta_type = IFLA_EXT_MASK;
    nl_req->rtattr.rta_len = 8;
    nl_req->rtattr.rta_val = RTEXT_FILTER_VF;

    int ret = nl_send_auto(socket, msg);
    printf("nl_send_auto returned %d\n", ret);

    nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, port_parse, NULL);
    nl_recvmsgs_default(socket);

    nlmsg_free(msg);
    nl_socket_free(socket);

    return 0;
}

#cgo CFLAGS: -g -Og -W -Wextra -Wno-unused-parameter -I/usr/include/libnl3
#cgo LDFLAGS: -lnl-route-3 -lnl-3 -lev
*/
import "C"
import "fmt"

//export net_port_add
func net_port_add(port *C.nl_port_mod_t) C.int {
	fmt.Println(port.mtu)
	return C.int(0)
}

func PortList() {
	C.port_list()
}
