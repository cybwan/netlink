#ifndef __FLB_LBRT_VLAN_H__
#define __FLB_LBRT_VLAN_H__

#include <lbrt/types.h>

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
  char name[IF_NAMESIZE];
  char zone[ZONE_NAMESIZE];
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

struct lbrt_port_hw_info;

lbrt_vlans_h_t *lbrt_vlans_h_new(struct lbrt_zone *zone);
void lbrt_vlans_h_free(struct lbrt_vlans_h *vh);

bool lbrt_vlan_valid(__u32 vlan_id);
int lbrt_vlan_add(struct lbrt_vlans_h *vh, __u32 vlan_id, char *name,
                  char *zone, __u32 osid, struct lbrt_port_hw_info *hwi);
int lbrt_vlan_del(lbrt_vlans_h_t *vh, __u32 vlan_id);

int lbrt_vlan_datapath(lbrt_vlan_t *vlan, enum lbrt_dp_work work);

#endif /* __FLB_LBRT_VLAN_H__ */