#ifndef __FLB_API_DEFS_H__
#define __FLB_API_DEFS_H__

#include <cmn/types.h>

typedef enum {
  PortReal = 1, // Base port type
  PortBondSif,  // Bond slave port type
  PortBond,     // Bond port type
  PortVlanSif,  // Vlan slave port type
  PortVlanBr,   // Vlan Br port type
  PortVxlanSif, // Vxlan slave port type
  PortVxlanBr,  // Vxlan br port type
  PortWg,       // Wireguard port type
  PortVti,      // Vti port type
  PortIPTun,    // IPInIP port type
  PortGre,      // GRE port type
} api_port_type_t;

typedef enum api_port_prop {
  PortPropUpp = 1, // User-plane processing enabled
  PortPropSpan,    //  SPAN is enabled
  PortPropPol,     //  -Policer is active
} api_port_prop_t;

typedef __u8 api_dp_status_t;

typedef struct api_port_stats_info { // stats information of port
  __u64 rx_bytes;                    // rx Byte count
  __u64 tx_bytes;                    // tx Byte count
  __u64 rx_packets;                  // tx Packets count
  __u64 tx_packets;                  // tx Packets count
  __u64 rx_error;                    // rx error count
  __u64 tx_error;                    // tx error count
} api_port_stats_info_t;

typedef struct api_port_hw_info {    // hw info of a port
  __u8 mac_addr[ETH_ALEN];           // mac address as byte array
  char mac_addr_str[IF_MACADDRSIZE]; // mac address in string format
  bool link;                         // lowerlayer state
  bool state;                        // administrative state
  __u32 mtu;                         // maximum transfer unit
  char master[IF_NAMESIZE];          // master of this port if any
  char real[IF_NAMESIZE];            // underlying real dev info if any
  __u32 tun_id;                      // tunnel info if any
} api_port_hw_info_t;

typedef struct api_port_layer3_info { // layer3 info of a port
  bool routed;                        // routed mode or not
  char ipv4_addr[56];                 // ipv4 address set
  char ipv6_addr[56];                 // ipv6 address set
} api_port_layer3_info_t;

typedef struct api_port_sw_info { // software specific info of a port
  __u32 osid;                     // interface id of an OS
  __u32 port_type;                // type of port
  enum api_port_prop port_prop;   // port property
  bool port_active;               // port enabled/disabled
  bool bpf_loaded;                // eBPF loaded or not flag
} api_port_sw_info_t;

typedef struct api_port_layer2_info { // layer2 info of a port
  bool is_pvid;                       // this vid is Pvid or not
  __u32 vid;                          // vid related to prot
} api_port_layer2_info_t;

typedef struct api_port_dump {      // Generic dump info of a port
  char name[IF_NAMESIZE];           // name of the port
  __u32 port_no;                    // port number
  char zone[ZONE_NAMESIZE];         // security zone info
  struct api_port_sw_info sinfo;    // software specific port information
  struct api_port_hw_info hinfo;    // hardware specific port information
  struct api_port_stats_info stats; // port statistics related information
  struct api_port_layer3_info l3;   // layer3 info related to port
  struct api_port_layer2_info l2;   // layer2 info related to port
  api_dp_status_t sync;             // sync state
} api_port_dump_t;

typedef struct api_port_mod { // port modification info
  char *dev;                  // name of port
  __u32 link_index;           // OS allocated index
  __u32 link_type;            // port type
  __u8 mac_addr[ETH_ALEN];    // mac address
  bool link;                  // lowerlayer state
  bool state;                 // administrative state
  __u32 mtu;                  // maximum transfer unit
  char *master;               // master of this port if any
  char *real;                 // underlying real dev info if any
  __u32 tun_id;               // tunnel info if any
  char *tun_src;              //  tunnel source
  char *tun_dst;              // tunnel dest
} api_port_mod_t;

typedef struct api_vlan_mod { // Info about a vlan
  __u32 vid;                  // vlan identifier
  char *dev;                  // name of the related device
  __u32 link_index;           // OS allocated index
  __u8 mac_addr[ETH_ALEN];    // mac address
  bool link;                  // lowerlayer state
  bool state;                 // administrative state
  __u32 mtu;                  // maximum transfer unit
  __u32 tun_id;               // tunnel info if any
} api_vlan_mod_t;

typedef struct api_vlan_port_mod { // Info about a port attached to a vlan
  __u32 vid;                       // vlan identifier
  char dev[IF_NAMESIZE];           // name of the related device
  bool tagged;                     // tagged or not
} api_vlan_port_mod_t;

typedef struct api_vlan_stat { // statistics for vlan interface
  __u64 in_bytes;
  __u64 in_packets;
  __u64 out_bytes;
  __u64 out_packets;
} api_vlan_stat_t;

typedef struct api_vlan_get {       // Info for vlan interface to get
  __u32 vid;                        // vlan identifier
  char dev[IF_NAMESIZE];            // name of port
  __u16 member_cnt;                 // count of slave ports
  struct api_vlan_port_mod *member; // name of slave ports
  struct api_vlan_stat stat;        // Vlan traffic statistics
} api_vlan_get_t;

enum {
  FdbPhy = 0,  // FdbPhy - fdb of a real dev
  FdbTun = 1,  // FdbTun - fdb of a tun dev
  FdbVlan = 2, // FdbVlan - fdb of a vlan dev
};

// FdbMod - Info about a forwarding data-base
typedef struct api_fdb_mod {
  __u8 mac_addr[ETH_ALEN]; // mac address
  __u32 bridge_id;         // bridge domain-id
  char *dev;               // name of the related device
  char *dst;               // ip addr related to fdb
  __u32 type;              // One of FdbPhy/FdbTun/FdbVlan
} api_fdb_mod_t;

typedef struct api_ip_addr_mod { // Info about an ip address
  char *dev;                     // name of the related device
  char *ip;                      // Actual IP address
} api_ip_addr_mod_t;

typedef struct api_neigh_mod {  // Info about an neighbor
  char ip[IF_ADDRSIZE];         // The IP address
  __u32 link_index;             // OS allocated index
  __u32 state;                  // active or inactive
  __u8 hardware_addr[ETH_ALEN]; // resolved hardware address if any
} api_neigh_mod_t;

typedef struct api_ip_addr_get { // Info about an ip addresses
  char *dev;                     // name of the related device
  __u16 ip_cnt;                  // Count of IP addresses
  char **ip;                     // Actual IP address
  api_dp_status_t sync;          // sync state
} api_ip_addr_get_t;

typedef struct api_route_get_entry_statistic { // Info about an route statistic
  __u32 bytes;   // Statistic of the ingress port bytes.
  __u32 packets; // Statistic of the egress port bytes.
} api_route_get_entry_statistic_t;

typedef struct api_route_get {                    // Info about an route
  __u32 protocol;                                 // Protocol type
  char flags[20];                                 // flag type
  char gw[IF_ADDRSIZE];                           // gateway information if any
  __u32 link_index;                               // OS allocated index
  char dst[IF_CIDRSIZE];                          // ip addr
  __u32 hardware_mark;                            // index of the route
  struct api_route_get_entry_statistic statistic; // statistic
  api_dp_status_t sync;                           // sync
} api_route_get_t;

typedef struct api_route_mod { // Info about a route
  __u32 protocol;              // Protocol type
  __u32 flags;                 // flag type
  char *gw;                    // gateway information if any
  __u32 link_index;            // OS allocated index
  char *dst;                   // ip addr
} api_route_mod_t;

typedef struct api_fw_opt_arg { // Information related to Firewall options
  bool drop;                    // Drop any matching rule
  bool trap;                    // Trap anything matching rule
  bool record;                  // Record packets matching rule
  bool rdr;                     // Redirect any matching rule
  char *rdr_port;               // Redirect port
  bool allow;                   // Allow any matching rule
  __u32 mark;                   // Mark the matching rule
} api_fw_opt_arg_t;

typedef struct api_fw_rule_arg { // Information related to firewall rule
  char *src_ip;                  // Source IP in CIDR notation
  char *dst_ip;                  // Destination IP in CIDR notation
  __u16 src_port_min;            // Minimum source port range
  __u16 src_port_max;            // Maximum source port range
  __u16 dst_port_min;            // Minimum destination port range
  __u16 dst_port_max;            // Maximum source port range
  __u8 proto;                    // the protocol
  char *in_port;                 // the incoming port
  __u16 pref;                    // User preference for ordering
} api_fw_rule_arg_t;

typedef struct api_fw_rule_mod { // Info related to a firewall entry
  struct api_fw_rule_arg *rule;  // service argument of type FwRuleArg
  struct api_fw_opt_arg *opts;   // firewall options
} api_fw_rule_mod_t;

typedef struct api_endpoint_mod { // Info related to a end-point entry
  char *hostname;                 // hostname in CIDR
  char *name;                     // Endpoint Identifier
  __u32 inact_tries;    // No. of inactive probes to mark an end-point inactive
  char *probe_type;     // Type of probe : "icmp","connect-tcp", "connect-udp",
                        // "connect-sctp", "http", "https"
  char *probe_req;      // Request string in case of http probe
  char *probe_resp;     // Response string in case of http probe
  __u32 probe_duration; // How frequently (in seconds) to check activity
  __u16 probe_port;     // Port to probe for connect type
  char *min_delay;      // Minimum delay in this end-point
  char *avg_delay;      // Average delay in this end-point
  char *max_delay;      // Max delay in this end-point
  char *curr_state;     // Current state of this end-point
} api_endpoint_mod_t;

typedef enum api_ep_select { // Selection method of load-balancer end-point
  LbSelRr = 0,               // select the lb end-points based on round-robin
  LbSelHash,                 // select the lb end-points based on hashing
  LbSelPrio,                 // select the lb based on weighted round-robin
} api_ep_select_t;

typedef enum api_lb_mode { // Variable to define LB mode
  LBModeDefault = 0,       // Default Mode(DNAT)
  LBModeOneArm,            // One Arm Mode
  LBModeFullNAT,           // Full NAT Mode
  LBModeDSR,               // DSR Mode
} api_lb_mode_t;

typedef struct api_lb_service_arg { // Information related to load-balancer
                                    // service
  char *serv_ip;   // the service ip or vip  of the load-balancer rule
  __u16 serv_port; // the service port of the load-balancer rule
  char *proto;     // the service protocol of the load-balancer rule
  __u16 block_num; // An arbitrary block num to further segregate a service
  enum api_ep_select sel; // one of LbSelRr,LbSelHash, or LbSelHash
  bool bgp;               // export this rule with goBGP
  bool monitor;           // monitor end-points of this rule
  enum api_lb_mode mode;  // NAT mode
  __u32 inactive_timeout; // Forced session reset after inactive timeout
  bool managed;           // This rule is managed by external entity e.g k8s
  char *probe_type; // Liveness check type for this rule : ping, tcp, udp, sctp,
                    // none, http(s)
  __u16 probe_port; // Liveness check port number. Only valid for tcp, udp,
                    // sctp, http(s)
  char *probe_req;  // Request string for liveness check
  char *probe_resp; // Response string for liveness check
  char *name;       // Service name
} api_lb_service_arg_t;

typedef struct api_lb_endpoint_arg { // Information related to load-balancer
                                     // end-point
  char *ep_ip;                       // endpoint IP address
  __u16 ep_port;                     // endpoint Port
  __u8 weight;    // weight associated with end-point; Only valid for weighted
                  // round-robin selection
  char *state;    // current state of the end-point
  char *counters; // traffic counters of the end-point
} api_lb_endpoint_arg_t;

typedef struct api_lb_sec_ip_arg { // Secondary IP
  char *sec_ip;                    // Secondary IP address
} api_lb_sec_ip_arg_t;

typedef struct api_lb_rule_mod {  // Info related to a load-balancer entry
  struct api_lb_service_arg serv; // service argument of type LbServiceArg
  __u16 sec_ips_cnt;
  struct api_b_sec_ip_arg *
      *sec_ips; // Secondary IPs for SCTP multi-homed service
  __u16 eps_cnt;
  struct api_lb_endpoint_arg **eps; // slice containing LbEndPointArg
} api_lb_rule_mod_t;

typedef struct api_ct_info { // Conntrack Information
  char *d_ip;                // destination ip address
  char *s_ip;                // source ip address
  __u16 d_port;              // destination port information
  __u16 s_port;              // source port information
  char *proto;               // IP protocol information
  char *c_state;             // current state of conntrack
  char *c_act;               // any related action
  __u64 pkts;                // packets tracked by ct entry
  __u64 bytes;               // bytes tracked by ct entry
  char *service_name;        // Connection's service name
} api_ct_info_t;

typedef struct api_ulcl_arg { // ulcl argument information
  char *addr;                 // filter ip addr
  __u8 qfi;                   // qfi id related to this filter
} api_ulcl_arg_t;

typedef struct api_sess_tun { // session tunnel(l3) information
  __u32 te_id;                // tunnel-id
  char *addr;                 // tunnel ip addr of remote-end
} api_sess_tun_t;

typedef struct api_param_mod { // Info related to a operational parameters
  char *log_level;             // log level of loxilb
} api_param_mod_t;

typedef struct api_gobgp_global_config { // Info related to goBGP global config
  __u64 local_as;                        // Local AS number
  char *router_id;                       // BGP Router ID
  bool set_nh_self;
  __u16 listen_port;
} api_gobgp_global_config_t;

typedef struct api_gobgp_neigh_mod { // Info related to goBGP neigh
  char *addr;
  __u32 remote_as;
  __u16 remote_port;
  bool multi_hop;
} api_gobgp_neigh_mod_t;

typedef struct api_session_mod { // information related to a user-session
  char *ident;                   // unique identifier for this session
  char *ip;                      // ip address of the end-user of this session
  struct api_sess_tun *an_tun;   // access tunnel network information
  struct api_sess_tun *cn_tun;   // core tunnel network information
} api_session_mod_t;

typedef struct api_session_ulcl_mod { // information related to a ulcl filter
  char *ident;               // identifier of the session for this filter
  struct api_ulcl_arg *args; // ulcl filter information
} api_session_ulcl_mod_t;

typedef struct api_has_mod { // information related to a cluster HA instance
  char *instance;            // Cluster Instance
  char *state;               // current HA state
  char *vip;                 // Instance virtual IP address
} api_has_mod_t;

typedef struct api_cluster_node_mod { // information related to a cluster node
                                      // instance
  char *addr;                         // Cluster Instance
} api_cluster_node_mod_t;

enum {
  PolTypeTrtcm = 0, // Default,Policer type trtcm
  PolTypeSrtcm = 1, // Policer type srtcm
};

typedef struct api_pol_info { // information related to a policer
  __u32 pol_type;             // one of PolTypeTrtcm or PolTypeSrtcm
  bool color_aware;           // color aware or not
  __u64 committed_info_rate;  // CIR in Mbps
  __u64 peak_info_rate;       // PIR in Mbps
  __u64 committed_blk_size;   // CBS in bytes 0 for default selection
  __u64 excess_blk_size;      // EBS in bytes 0 for default selection
} api_pol_info_t;

typedef enum api_pol_obj_type { // type  of a policer attachment object
  PolAttachPort = 1,            // attach policer to port
  PolAttachLbRule,              // attach policer to a rule
} api_pol_obj_type_t;

typedef struct api_pol_obj { // Information related to policer attachment point
  char pol_obj_name[POL_NAMESIZE];  // name of the object
  enum api_pol_obj_type attachment; // attach point type of the object
} api_pol_obj_t;

typedef struct api_pol_mod {  // Information related to policer entry
  char *ident;                // identifier
  struct api_pol_info *info;  // policer info of type PolInfo
  struct api_pol_obj *target; // target object information
} api_pol_mod_t;

enum {
  MirrTypeSpan = 0,   // Default, simple SPAN
  MirrTypeRspan = 1,  // type RSPAN
  MirrTypeErspan = 2, // type ERSPAN
};

typedef struct api_mirr_info { // information related to a mirror entry
  __u32 mirr_type; // one of MirrTypeSpan, MirrTypeRspan or MirrTypeErspan
  char mirr_port[IF_NAMESIZE]; // port where mirrored traffic needs to be sent
  __u32 mirr_vlan; // for RSPAN we may need to send tagged mirror traffic
  char mirr_r_ip[IF_ADDRSIZE]; // RemoteIP. For ERSPAN we may need to send
                               // tunnelled mirror traffic
  char mirr_s_ip[IF_ADDRSIZE]; // SourceIP. For ERSPAN we may need to send
                               // tunnelled mirror traffic
  __u32 mirr_tid; // mirror tunnel-id. For ERSPAN we may need to send tunnelled
                  // mirror traffic
} api_mirr_info_t;

typedef enum api_mirr_obj_type {
  MirrAttachPort = 1, // mirror attachment to a port
  MirrAttachRule,     // mirror attachment to a lb rule
} api_mirr_obj_type_t;

typedef struct api_mirr_obj {        // information of object attached to mirror
  char mirr_obj_name[MIRR_NAMESIZE]; // object name to be attached to mirror
  enum api_mirr_obj_type attachment; // one of MirrAttachPort or MirrAttachRule
} api_mirr_obj_t;

typedef struct api_mirr_mod { // information related to a  mirror entry
  char *ident;                // unique identifier for the mirror
  struct api_mirr_info *info; // information about the mirror
  struct api_mirr_obj
      *target; // information about object to which mirror needs to be attached
} api_mirr_mod_t;

typedef struct api_mirr_get { // information related to Get a mirror entry
  char ident[MIRR_NAMESIZE];  // unique identifier for the mirror
  struct api_mirr_info info;  // information about the mirror
  struct api_mirr_obj
      target; // information about object to which mirror needs to be attached
  api_dp_status_t sync; // sync state
} api_mirr_get_t;

typedef struct api_net_hook {
  // net_mirror_get() ([]mirrgetmod, error)
  // net_mirror_add(*mirrmod) (int, error)
  // net_mirror_del(*mirrmod) (int, error)
  // net_port_get() ([]portdump, error)
  // net_port_add(*portmod) (int, error)
  // net_port_del(*portmod) (int, error)
  // net_vlan_get() ([]vlanget, error)
  // net_vlan_add(*vlanmod) (int, error)
  // net_vlan_del(*vlanmod) (int, error)
  // net_vlan_port_add(*vlanportmod) (int, error)
  // net_vlan_port_del(*vlanportmod) (int, error)
  // net_fdb_add(*fdbmod) (int, error)
  // net_fdb_del(*fdbmod) (int, error)
  // net_addr_get() ([]ipaddrget, error)
  // net_addr_add(*ipaddrmod) (int, error)
  // net_addr_del(*ipaddrmod) (int, error)
  // net_neigh_get() ([]neighmod, error)
  // net_neigh_add(*neighmod) (int, error)
  // net_neigh_del(*neighmod) (int, error)
  // net_route_get() ([]routeget, error)
  // net_route_add(*routemod) (int, error)
  // net_route_del(*routemod) (int, error)
  // net_lb_rule_add(*lbrulemod) (int, error)
  // net_lb_rule_del(*lbrulemod) (int, error)
  // net_lb_rule_get() ([]lbrulemod, error)
  // net_ct_info_get() ([]ctinfo, error)
  // net_session_get() ([]sessionmod, error)
  // net_session_add(*sessionmod) (int, error)
  // net_session_del(*sessionmod) (int, error)
  // net_session_ulcl_get() ([]sessionulclmod, error)
  // net_session_ulcl_add(*sessionulclmod) (int, error)
  // net_session_ulcl_del(*sessionulclmod) (int, error)
  // net_policer_get() ([]polmod, error)
  // net_policer_add(*polmod) (int, error)
  // net_policer_del(*polmod) (int, error)
  // net_ci_state_mod(*hasmod) (int, error)
  // net_ci_state_get() ([]hasmod, error)
  // net_fw_rule_add(*fwrulemod) (int, error)
  // net_fw_rule_del(*fwrulemod) (int, error)
  // net_fw_rule_get() ([]fwrulemod, error)
  // net_ep_host_add(fm *endpointmod) (int, error)
  // net_ep_host_del(fm *endpointmod) (int, error)
  // net_ep_host_get() ([]endpointmod, error)
  // net_param_set(param parammod) (int, error)
  // net_param_get(param *parammod) (int, error)
  // net_gobgp_neigh_add(nm *gobgpneighmod) (int, error)
  // net_gobgp_neigh_del(nm *gobgpneighmod) (int, error)
  // net_gobgp_gc_add(gc *gobgpglobalconfig) (int, error)
} api_net_hook_t;

#endif /* __FLB_API_DEFS_H__ */