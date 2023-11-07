#include <lbrt/vlan.h>

extern struct lbrt_net_meta mh;

lbrt_vlans_h_t *lbrt_vlans_h_alloc(lbrt_zone_t *zone) {
  lbrt_vlans_h_t *vh;
  vh = calloc(1, sizeof(*vh));
  if (!vh) {
    return NULL;
  }
  vh->zone = zone;
  return vh;
}

void lbrt_vlans_h_free(lbrt_vlans_h_t *vh) {
  if (!vh)
    return;
  free(vh);
}

bool lbrt_vlan_valid(__u32 vlan_id) {
  return vlan_id > 0 && vlan_id < MaximumVlans - 1;
}

int lbrt_vlan_add(lbrt_vlans_h_t *vh, __u32 vlan_id, char *name, char *zone,
                  __u32 osid, lbrt_port_hw_info_t *hwi) {
  if (!lbrt_vlan_valid(vlan_id)) {
    return VLAN_RANGE_ERR;
  }

  lbrt_vlan_t *vlan = vh->vlan_map[vlan_id];
  if (vlan && vlan->created)
    return VLAN_EXISTS_ERR;

  int ret = lbrt_zone_br_add(mh.zn, name, zone);
  if (ret != 0)
    return VLAN_EXISTS_ERR;

  lbrt_port_layer2_info_t l2i;
  memset(&l2i, 0, sizeof(lbrt_port_layer2_info_t));
  l2i.is_p_vid = false;
  l2i.vid = vlan_id;
  ret = lbrt_port_add(vh->zone->ports, name, osid, PortVlanBr, zone, hwi, &l2i);
  if (ret != 0) {
    lbrt_zone_br_del(mh.zn, name);
    return VLAN_ADD_BRP_ERR;
  }

  vlan = calloc(1, sizeof(lbrt_vlan_t));
  vlan->vlan_id = vlan_id;
  memcpy(vlan->name, name, strlen(name));
  memcpy(vlan->zone, zone, strlen(zone));
  vlan->created = true;

  vh->vlan_map[vlan_id] = vlan;

  flb_log(LOG_LEVEL_INFO, "vlan %d bd created", vlan_id);

  return 0;
}

int lbrt_vlan_del(lbrt_vlans_h_t *vh, __u32 vlan_id) {
  if (!lbrt_vlan_valid(vlan_id)) {
    return VLAN_RANGE_ERR;
  }

  lbrt_vlan_t *vlan = vh->vlan_map[vlan_id];
  if (vlan || !vlan->created)
    return VLAN_NOT_EXIST_ERR;

  if (vlan->num_tag_ports != 0 || vlan->num_un_tag_ports != 0) {
    return VLAN_MP_EXIST_ERR;
  }

  lbrt_zone_br_del(mh.zn, vlan->name);

  lbrt_port_del(vh->zone->ports, vlan->name, PortVlanBr);
  lbrt_vlan_datapath(vlan,DP_STATSCLR);

  vh->vlan_map[vlan_id] = NULL;
  free(vlan);

  flb_log(LOG_LEVEL_INFO, "vlan %d bd deleted", vlan_id);

  return 0;
}

int lbrt_vlan_datapath(lbrt_vlan_t *vlan, enum lbrt_dp_work work) { return 0; }