#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

static int callback(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct ifinfomsg *iface = NLMSG_DATA(nlh);
    struct rtattr *hdr = IFLA_RTA(iface);
    int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));

    //printf("Got something.\n");
    //nl_msg_dump(msg, stdout);

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

    return NL_OK;
}

int main() {
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

    nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, callback, NULL);
    nl_recvmsgs_default(socket);

    return 0;
}