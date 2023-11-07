#ifndef __FLB_LBRT_PORT_H__
#define __FLB_LBRT_PORT_H__

#include <lbrt/types.h>

#define MaxBondInterfaces 8
#define MaxRealInterfaces 128
#define MaxInterfaces 512
#define MaxWgInterfaces 8
#define MaxVtiInterfaces 8
#define RealPortIDB 3800
#define BondIDB 4000
#define WgIDB 4010
#define VtIDB 4020

typedef enum lbrt_port_event {
  PortEvDown = 1,
  PortEvLowerDown = 2,
  PortEvDelete = 4,
} lbrt_port_event_t;

typedef struct lbrt_port_event_intf {
} lbrt_port_event_intf_t;

typedef struct lbrt_port_stats_info {
  __u64 rx_bytes;
  __u64 tx_bytes;
  __u64 rx_packets;
  __u64 tx_packets;
  __u64 rx_error;
  __u64 tx_error;
} lbrt_port_stats_info_t;

typedef struct lbrt_port_hw_info {
  __u8 mac_addr[ETH_ALEN];
  bool link;
  bool state;
  __u32 mtu;
  char master[IF_NAMESIZE];
  char real[IF_NAMESIZE];
  __u32 tun_id;
  ip_t tun_src;
  ip_t tun_dst;
} lbrt_port_hw_info_t;

typedef struct lbrt_port_layer3_info {
  bool routed;
  // char **ipv4_addrs;
  // char **ipv6_addrs;
} lbrt_port_layer3_info_t;

typedef struct lbrt_port_sw_info {
  __u32 os_id;
  __u32 port_type;
  enum PortProp PortProp;
  __u32 port_pol_num;
  __u32 port_mir_num;
  bool port_active;
  struct lbrt_port *port_real;
  struct lbrt_port *port_ovl;
  __u64 sess_mark;
  bool bpf_loaded;
} lbrt_port_sw_info_t;

typedef struct lbrt_port_layer2_info {
  bool is_p_vid;
  __u32 vid;
} lbrt_port_layer2_info_t;

typedef struct lbrt_port {
  char name[IF_NAMESIZE];
  __u32 osid;
  __u32 port_no;
  char zone[ZONE_NAMESIZE];
  struct lbrt_port_sw_info sinfo;
  struct lbrt_port_hw_info hinfo;
  struct lbrt_port_stats_info stats;
  struct lbrt_port_layer3_info l3;
  struct lbrt_port_layer2_info l2;
  enum lbrt_dp_status sync;

  UT_hash_handle hh_by_name;
  UT_hash_handle hh_by_osid;

} lbrt_port_t;

typedef struct lbrt_ports_h {
  struct lbrt_port *port_i_map[MaxInterfaces];
  struct lbrt_port *port_s_map;
  struct lbrt_port *port_o_map;
  struct lbrt_port_event_intf *port_notifs;
  struct lbrt_counter *port_mark;
  struct lbrt_counter *bond_mark;
  struct lbrt_counter *wg_mark;
  struct lbrt_counter *vti_mark;
} lbrt_ports_h_t;

lbrt_ports_h_t *lbrt_ports_h_alloc(void);
void lbrt_ports_h_free(lbrt_ports_h_t *ph);

lbrt_port_t *lbrt_port_find_by_name(lbrt_ports_h_t *ph, const char *name);
lbrt_port_t *lbrt_port_find_by_osid(lbrt_ports_h_t *ph, __u32 osid);
int lbrt_port_add(lbrt_ports_h_t *ph, char *name, __u32 osid, __u32 link_type,
                  char *zone, lbrt_port_hw_info_t *hwi,
                  lbrt_port_layer2_info_t *l2i);
int lbrt_port_del(lbrt_ports_h_t *ph, char *name, __u32 link_type);

int lbrt_port_datapath(lbrt_port_t *port, enum lbrt_dp_work work);

#endif /* __FLB_LBRT_PORT_H__ */