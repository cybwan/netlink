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

lbrt_neigh_tun_ep_t *lbrt_neigh_add_tun_ep(lbrt_neigh_h_t *nhh,
                                           lbrt_neigh_t *nh, ip_t *rip,
                                           ip_t *sip, __u32 tun_id,
                                           lbrt_dp_tun_t tun_type, bool sync) {
  lbrt_port_t *port = nh->o_if_port;
  if (!port || (port->sinfo.port_ovl == NULL && tun_type != DP_TUN_IPIP)) {
    return NULL;
  }

  lbrt_neigh_tun_ep_t *tep = NULL;

  __u32 tep_cnt = utarray_len(nh->tun_eps);
  for (__u32 i = 0; i < tep_cnt; i++) {
    tep = (lbrt_neigh_tun_ep_t *)utarray_eltptr(nh->tun_eps, i);
    if (memcmp(&tep->r_ip, rip, sizeof(ip_t)) == 0 && tep->tun_id == tun_id &&
        tep->tun_type == tun_type) {
      return tep;
    }
  }

  if (!sip || (!sip->f.v4 && !sip->f.v6)) {
    ip_t s_ip;
    char dev[IF_NAMESIZE];
    int ret =
        lbrt_ifa_select(nhh->zone->l3, port->name, rip, false, &s_ip, dev);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "%s:ifa select error", port->name);
      return NULL;
    }
    sip = &s_ip;
  }

  __u64 idx = lbrt_counter_get_counter(nhh->neigh_tid);
  if (idx == COUNTER_OVERFLOW) {
    return NULL;
  }

  tep = calloc(1, sizeof(lbrt_neigh_tun_ep_t));
  tep->tun_id = tun_id;
  tep->tun_type = tun_type;
  tep->mark = idx;
  tep->parent = nh;
  memcpy(&tep->r_ip, rip, sizeof(ip_t));
  memcpy(&tep->s_ip, sip, sizeof(ip_t));

  utarray_push_back(nh->tun_eps, tep);
  free(tep);
  tep = (lbrt_neigh_tun_ep_t *)utarray_back(nh->tun_eps);

  nh->type |= NH_TUN;

  if (sync) {
    lbrt_neigh_tun_ep_datapath(tep, DP_CREATE);
  }

  char sip_addr[IF_ADDRSIZE];
  char rip_addr[IF_ADDRSIZE];
  ip_ntoa(sip, sip_addr);
  ip_ntoa(rip, sip_addr);

  flb_log(LOG_LEVEL_DEBUG, "neigh tunep added - %s:%s (%d)\n", sip_addr,
          rip_addr, tun_id);

  return tep;
}

void lbrt_neigh_del_tun_ep(lbrt_neigh_t *nh, __u32 idx) {
  utarray_erase(nh->tun_eps, idx, 1);
}

void lbrt_neigh_del_all_tun_ep(lbrt_neigh_h_t *nhh, lbrt_neigh_t *nh) {
  __u32 tep_cnt = utarray_len(nh->tun_eps);
  if (tep_cnt > 0) {
    lbrt_neigh_tun_ep_t *tep = NULL;
    for (__s32 idx = tep_cnt - 1; idx >= 0; idx--) {
      tep = (lbrt_neigh_tun_ep_t *)utarray_eltptr(nh->tun_eps, (__u32)idx);
      lbrt_neigh_tun_ep_datapath(tep, DP_REMOVE);
      lbrt_counter_put_counter(nhh->neigh_tid, tep->mark);
      tep->in_active = true;
      lbrt_neigh_del_tun_ep(nh, (__u32)idx);
    }
  }
}

int lbrt_neigh_del_by_port(lbrt_neigh_h_t *nh, const char *port) { return 0; }

int lbrt_neigh_tun_ep_datapath(lbrt_neigh_tun_ep_t *tep,
                               enum lbrt_dp_work work) {
  return 0;
}