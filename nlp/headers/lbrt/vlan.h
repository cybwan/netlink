#ifndef __FLB_LBRT_VLAN_H__
#define __FLB_LBRT_VLAN_H__

#include <lbrt/types.h>

enum {
  VLAN_BASE_ERR = -2000,
  VLAN_EXISTS_ERR,
  VLAN_NOT_EXIST_ERR,
  VLAN_RANGE_ERR,
  VLAN_ADD_BRP_ERR,
  VLAN_MP_EXIST_ERR,
  VLAN_PORT_PHY_ERR,
  VLAN_PORT_EXIST_ERR,
  VLAN_PORT_TAGGED_ERR,
  VLAN_NO_PORT_ERR,
  VLAN_PORT_CREATE_ERR,
  VLAN_ZONE_ERR,
};

#define MaximumVlans 4096 // constant to declare maximum number of vlans

typedef struct lbrt_vlan_stat {
  __u64 in_bytes;
  __u64 in_packets;
  __u64 out_bytes;
  __u64 out_packets;
} lbrt_vlan_stat_t;

typedef struct lbrt_vlan {
  __u32 vlan_id;
  bool created;
  char *name;
  char *zone;
  __u32 num_tag_ports;
  struct lbrt_port *tagged_ports[MaximumVlans];
  __u32 num_un_tag_ports;
  struct lbrt_port *un_tagged_ports[MaximumVlans];
  struct lbrt_vlan_stat stat;
} lbrt_vlan_t;

typedef struct lbrt_vlans_h {
  struct lbrt_vlan *vlan_map[MaximumVlans];
  struct lbrt_zone *zone;
} lbrt_vlans_h_t;

lbrt_vlans_h_t *lbrt_vlans_h_alloc(lbrt_zone_t *zone);
void lbrt_vlans_h_free(lbrt_vlans_h_t *vh);

bool lbrt_vlan_valid(__u32 vlan_id);

#endif /* __FLB_LBRT_VLAN_H__ */