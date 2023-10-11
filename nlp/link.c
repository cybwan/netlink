#include <nlp.h>

static inline void debug_link(nl_port_mod_t *port) {
    printf("Master: %2d Index: %2d MTU:%5d "
        "MAC: %02x:%02x:%02x:%02x:%02x:%02x "
        "State:%d IFNAME: %8s ", 
        port->master_index, port->index, port->mtu, 
        port->hwaddr[0], port->hwaddr[1], port->hwaddr[2], port->hwaddr[3], port->hwaddr[4], port->hwaddr[5], 
        port->oper_state, port->name);
    if(port->type.vxlan) {
        printf("Type: vxlan vxlan_id: %3d vtep_dev_index: %d", port->u.vxlan.vxlan_id ,port->u.vxlan.vtep_dev_index);
    } else if(port->type.bridge) {
        printf("Type: bridge");
    } else if(port->type.bond) {
        printf("Type: bond");
    } else if(port->type.ipip) {
        printf("Type: iptun");
    }
    printf("\n");
}

int link_callback(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct ifinfomsg *link_msg = NLMSG_DATA(nlh);
    struct rtattr* attrs[IFLA_MAX+1];
    nl_port_mod_t port;

    memset(&port, 0, sizeof(port));

    int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*link_msg));

    parse_rtattr(attrs, IFLA_MAX, IFLA_RTA(link_msg), remaining);

    //printf("Got something.\n");
    //nl_msg_dump(msg, stdout);

    port.index = link_msg->ifi_index;
    port.flags = link_msg->ifi_flags;
    

    if (attrs[IFLA_MASTER]) {
        port.master_index = *(__u32 *)RTA_DATA(attrs[IFLA_MASTER]);
    }

    if (attrs[IFLA_IFNAME]) {
        __u8 *ifname = (__u8 *)RTA_DATA(attrs[IFLA_IFNAME]);
        memcpy(port.name, ifname, attrs[IFLA_IFNAME]->rta_len);
    }

    if (attrs[IFLA_MTU]) {
        port.mtu = *(__u32 *)RTA_DATA(attrs[IFLA_MTU]);
    }

    if (attrs[IFLA_OPERSTATE]) {
        port.oper_state = *(__u8 *)RTA_DATA(attrs[IFLA_OPERSTATE]);
    }

    if (attrs[IFLA_ADDRESS]) {
        __u8 *hwaddr = (__u8 *)RTA_DATA(attrs[IFLA_ADDRESS]);
        memcpy(port.hwaddr, hwaddr, attrs[IFLA_ADDRESS]->rta_len);
    }

    struct rtattr *info = attrs[IFLA_LINKINFO];
    if (info) {
        struct rtattr* info_attrs[IFLA_INFO_MAX+1];
        parse_rtattr(info_attrs, IFLA_INFO_MAX, RTA_DATA(info), RTA_PAYLOAD(info));
        char *kind =  (char *)RTA_DATA(info_attrs[IFLA_INFO_KIND]);
        if (strcmp(kind, "bridge") == 0) {
            port.type.bridge = 1;
        } else if (strcmp(kind, "bond") == 0) {
            port.type.bond = 1;
        } else if (strcmp(kind, "vxlan") == 0) {
            struct rtattr *info_data = info_attrs[IFLA_INFO_DATA];
            struct rtattr* vxlan_attrs[IFLA_VXLAN_MAX+1];
            parse_rtattr(vxlan_attrs, IFLA_VXLAN_MAX, RTA_DATA(info_data), RTA_PAYLOAD(info_data));
            if (vxlan_attrs[IFLA_VXLAN_ID]) {
                port.u.vxlan.vxlan_id = *(__u32 *)RTA_DATA(vxlan_attrs[IFLA_VXLAN_ID]);
            }
            if (vxlan_attrs[IFLA_VXLAN_LINK]) {
                port.u.vxlan.vtep_dev_index = *(__u32 *)RTA_DATA(vxlan_attrs[IFLA_VXLAN_LINK]);
            }
            port.type.vxlan = 1;
        } else if (strcmp(kind, "ipip") == 0) {
            struct rtattr *info_data = info_attrs[IFLA_INFO_DATA];
            struct rtattr* iptun_attrs[IFLA_IPTUN_MAX+1];
            parse_rtattr(iptun_attrs, IFLA_IPTUN_MAX, RTA_DATA(info_data), RTA_PAYLOAD(info_data));
            if (iptun_attrs[IFLA_IPTUN_LOCAL]) {
                port.u.iptun.local = *(__u32 *)RTA_DATA(iptun_attrs[IFLA_IPTUN_LOCAL]);
            }
            if (iptun_attrs[IFLA_IPTUN_REMOTE]) {
                port.u.iptun.remote = *(__u32 *)RTA_DATA(iptun_attrs[IFLA_IPTUN_REMOTE]);
            }
            port.type.ipip = 1;
        }
    }

    debug_link(&port);

    return NL_OK;
}

int link_list() {
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

    nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, link_callback, NULL);
    nl_recvmsgs_default(socket);

    nlmsg_free(msg);
    nl_socket_free(socket);

    return 0;
}


int link_subscribe() {
    struct nl_sock *socket = nl_socket_alloc();
    nl_socket_disable_seq_check(socket);
    nl_socket_modify_cb(socket, NL_CB_VALID, NL_CB_CUSTOM, link_callback, NULL);
    nl_connect(socket, NETLINK_ROUTE);
    nl_socket_add_memberships(socket, RTNLGRP_LINK, 0);
    while(1) {
        printf("nl_recvmsgs_default\n");
        nl_recvmsgs_default(socket);
    }
    nl_socket_free(socket);
    return 0;
}

int main() {
    link_list();
    return 0;
}