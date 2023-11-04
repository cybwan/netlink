#include <lbrt/vlan.h>

extern struct lbrt_net_meta *mh;

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