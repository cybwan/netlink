#include <lbrt/neigh.h>

extern struct lbrt_net_meta mh;

#define NeighAts 10
#define MaxSysNeigh 3 * 1024
#define MaxTunnelNeigh 1024

UT_icd lbrt_neigh_tun_ep_icd = {sizeof(lbrt_neigh_tun_ep_t), NULL, NULL, NULL};

lbrt_neigh_t *lbrt_neigh_new() {
  lbrt_neigh_t *nh = calloc(1, sizeof(lbrt_neigh_t));
  utarray_new(nh->tun_eps, &lbrt_neigh_tun_ep_icd);
  return nh;
}

void lbrt_neigh_free(lbrt_neigh_t *nh) {
  if (!nh)
    return;
  lbrt_rt_t *rt, *tmp;
  HASH_ITER(hh, nh->nh_rt_m, rt, tmp) { HASH_DEL(nh->nh_rt_m, rt); }
  utarray_free(nh->tun_eps);
  free(nh);
}

lbrt_neigh_h_t *lbrt_neigh_h_new(lbrt_zone_t *zone) {
  lbrt_neigh_h_t *nhh;
  nhh = calloc(1, sizeof(*nhh));
  if (!nhh) {
    return NULL;
  }
  nhh->neigh_id = lbrt_counter_new(1, MaxSysNeigh);
  nhh->neigh_tid = lbrt_counter_new(MaxSysNeigh + 1, MaxTunnelNeigh);
  nhh->zone = zone;
  return nhh;
}

void lbrt_neigh_h_free(lbrt_neigh_h_t *nhh) {
  if (!nhh)
    return;
  if (nhh->neigh_id) {
    free(nhh->neigh_id);
  }
  if (nhh->neigh_tid) {
    free(nhh->neigh_tid);
  }
  lbrt_neigh_t *nh, *tmp;
  HASH_ITER(hh, nhh->neigh_map, nh, tmp) {
    HASH_DEL(nhh->neigh_map, nh);
    lbrt_neigh_free(nh);
  }
  free(nhh);
}

lbrt_neigh_t *lbrt_neigh_find(lbrt_neigh_h_t *nhh, const char *addr,
                              const char *zone) {
  lbrt_neigh_t *nh = NULL;
  lbrt_neigh_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(&key.nh, addr, strlen(addr));
  memcpy(&key.zone, zone, strlen(zone));
  HASH_FIND(hh, nhh->neigh_map, &key, sizeof(key), nh);
  return nh;
}

void lbrt_neigh_activate(lbrt_neigh_h_t *nhh, lbrt_neigh_t *nh) {
  if (nh->addr.f.v6) {
    return;
  }

  if (nh->resolved) {
    return;
  }

  lbrt_time_t now;
  lbrt_time_now(&now);
  if (lbrt_time_sub(&now, &nh->ats) < NeighAts ||
      strcmp(nh->o_if_port->name, "lo") == 0) {
    return;
  }

  ip_t sip;
  char dev[IF_NAMESIZE];
  int ret = lbrt_ifa_select(nhh->zone->l3, nh->o_if_port->name, &nh->addr, true,
                            &sip, dev);
  if (ret < 0) {
    flb_log(LOG_LEVEL_DEBUG, "Failed to select l3 ifa select");
    return;
  }

  lbrt_arp_ping(&nh->addr, &sip, nh->o_if_port->name);

  lbrt_time_now(&nh->ats);
}

int lbrt_neigh_del_by_port(lbrt_neigh_h_t *nh, const char *port) { return 0; }