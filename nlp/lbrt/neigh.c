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

bool lbrt_neigh_recursive_resolve(lbrt_neigh_h_t *nhh, lbrt_neigh_t *nh) {
  bool chg = false;
  lbrt_port_t *port = nh->o_if_port;
  if (!port) {
    return chg;
  }

  lbrt_neigh_attr_t *attr = &nh->attr;
  if (is_zero_mac(attr->hardware_addr)) {
    nh->resolved = false;
  } else {
    nh->resolved = true;
  }

  if (nh->t_fdb && (nh->t_fdb->inactive || nh->t_fdb->unreach)) {
    nh->resolved = false;
    nh->type &= ~NH_RECURSIVE;
    nh->t_fdb = NULL;
    nh->r_mark = 0;
  }

  if (nh->resolved) {
    if (lbrt_port_is_l3_tun_port(port)) {
      char tun_dst_addr[IF_ADDRSIZE];
      ip_ntoa(&port->hinfo.tun_dst, tun_dst_addr);
      ip_net_t dst_net;
      lbrt_trie_data_t t_data;
      int ret =
          lbrt_trie_find(nhh->zone->rt->trie4, tun_dst_addr, &dst_net, &t_data);
      if (ret == 0 && (dst_net.ip.f.v4 || dst_net.ip.f.v6)) {
        if (!t_data.f.neigh) {
          nh->resolved = false;
          nh->r_mark = 0;
          return false;
        }

        if (!t_data.v.neigh) {
          nh->resolved = false;
          nh->r_mark = 0;
          return false;
        }

        if (!t_data.v.neigh->in_active) {
          char dst_net_addr[IF_CIDRSIZE];
          ip_net_ntoa(&dst_net, dst_net_addr);
          lbrt_rt_t *rt =
              lbrt_rt_find(nhh->zone->rt, dst_net_addr, nhh->zone->name);
          if (!rt) {
            nh->resolved = false;
            nh->r_mark = 0;
            return false;
          }
          if (nh->r_mark == 0 || !nh->rec_nh || nh->rec_nh != nh) {
            flb_log(LOG_LEVEL_DEBUG, "IPTun-NH for %s:%s", tun_dst_addr,
                    nh->key.nh);
            lbrt_neigh_tun_ep_t *tep = lbrt_neigh_add_tun_ep(
                nhh, nh, &port->hinfo.tun_dst, &port->hinfo.tun_src,
                port->hinfo.tun_id, DP_TUN_IPIP, true);
            if (tep) {
              lbrt_rt_dep_obj_t dep;
              memset(&dep, 0, sizeof(dep));
              dep.f.neigh = true;
              dep.v.neigh = t_data.v.neigh;
              utarray_push_back(rt->rt_dep_objs, &dep);
              nh->r_mark = tep->mark;
              nh->resolved = true;
              nh->rec_nh = t_data.v.neigh;
              nh->type |= NH_RECURSIVE;
            }
            return true;
          }
        }
      }
      return false;
    }

    lbrt_fdb_key_t key;
    memset(&key, 0, sizeof(key));
    memcpy(&key.mac_addr, attr->hardware_addr, ETH_ALEN);
    key.bridge_id = port->l2.vid;

    lbrt_fdb_t *f = lbrt_fdb_find(nhh->zone->l2, &key);
    if (!f) {
      bool hasTun =
          lbrt_port_has_tun_slaves(nhh->zone->ports, port->name, PortVxlanSif);
      if (hasTun) {
        nh->t_fdb = NULL;
        nh->resolved = false;
        nh->r_mark = 0;
      }
    } else {
      if (f->fdb_attr.type == FdbTun) {
        if (f->unreach || !f->fdb_tun.ep) {
          nh->resolved = false;
          nh->r_mark = 0;
        } else {
          if (nh->t_fdb != f) {
            nh->t_fdb = f;
            nh->r_mark = f->fdb_tun.ep->mark;
            chg = true;
          } else if (nh->r_mark != f->fdb_tun.ep->mark) {
            nh->r_mark = f->fdb_tun.ep->mark;
            chg = true;
          }
          nh->type |= NH_RECURSIVE;
        }
      }
    }
  }

  return chg;
}

int lbrt_neigh_add(lbrt_neigh_h_t *nhh, ip_t *addr, const char *zone,
                   lbrt_neigh_attr_t *attr) {
  char addr_str[IF_ADDRSIZE];
  ip_ntoa(addr, addr_str);
  lbrt_port_t *port =
      lbrt_port_find_by_osid(nhh->zone->ports, attr->os_link_index);
  if (!port) {
    flb_log(LOG_LEVEL_ERR, "neigh add - %s:%s no oport\n", addr_str, zone);
    return NEIGH_OIF_ERR;
  }

  if (lbrt_port_is_l3_tun_port(port)) {
    mac_pton("00:11:22:33:44:55", attr->hardware_addr);
  }

  lbrt_neigh_t *nh = lbrt_neigh_find(nhh, addr_str, zone);
  if (nh) {
    nh->in_active = false;
    if (is_zero_mac(attr->hardware_addr)) {
      nh->resolved = true;
    } else {
      if (memcmp(attr->hardware_addr, nh->attr.hardware_addr, ETH_ALEN) != 0 ||
          !nh->resolved) {
        memcpy(nh->attr.hardware_addr, attr->hardware_addr, ETH_ALEN);
        nh->resolved = true;
        lbrt_neigh_recursive_resolve(nhh, nh);
        flb_log(LOG_LEVEL_DEBUG, "nh update - %s:%s (%d)", addr_str, zone,
                nh->resolved);
        lbrt_neigh_datapath(nh, DP_CREATE);
        goto NhExist;
      }
    }
    flb_log(LOG_LEVEL_ERR, "nh add - %s:%s exists", addr_str, zone);
    return NEIGH_EXISTS_ERR;
  }

  __u64 idx = lbrt_counter_get_counter(nhh->neigh_id);
  if (idx == COUNTER_OVERFLOW) {
    flb_log(LOG_LEVEL_ERR, "neigh add - %s:%s no marks", addr_str, zone);
    return NEIGH_RANGE_ERR;
  }

  nh = lbrt_neigh_new();
  memcpy(nh->key.nh, addr_str, strlen(addr_str));
  memcpy(nh->key.zone, zone, strlen(zone));
  memcpy(&nh->addr, addr, sizeof(ip_t));
  nh->o_if_port = port;
  nh->mark = idx;
  nh->type |= NH_NORMAL;
  nh->in_active = false;

  lbrt_neigh_recursive_resolve(nhh, nh);
  HASH_ADD(hh, nhh->neigh_map, key, sizeof(lbrt_neigh_key_t), nh);
  lbrt_neigh_datapath(nh, DP_CREATE);

NhExist:

  flb_log(LOG_LEVEL_DEBUG, "nh exists - %s:%s", addr_str, zone);

  ip_net_t ip_net;
  memcpy(&ip_net.ip, addr, sizeof(ip_t));
  if (addr->f.v4) {
    ip_net.mask = 32;
  } else {
    ip_net.mask = 128;
  }
  char ip_net_str[IF_CIDRSIZE];
  ip_net_ntoa(&ip_net, ip_net_str);

  lbrt_rt_attr_t ra;
  memset(&ra, 0, sizeof(ra));
  ra.protocol = 0;
  ra.os_flags = 0;
  ra.host_route = true;
  ra.ifi = attr->os_link_index;

  lbrt_rt_nh_attr_t na[1];
  memset(&na[0], 0, sizeof(na[0]));
  na[0].link_index = attr->os_link_index;
  memcpy(na[0].nh_addr, addr_str, strlen(addr_str));

  // Add a host specific to this neighbor
  int ret = lbrt_rt_add(nhh->zone->rt, ip_net_str, zone, &ra, 1, na);
  if (ret < 0 && ret != RT_EXISTS_ERR) {
    lbrt_neigh_del(nhh, addr, zone);
    flb_log(LOG_LEVEL_ERR, "neigh add - %s:%s host-rt fail(%s)", addr_str, zone,
            ret);
    return NEIGH_HOST_RT_ERR;
  }

  // Add a related L2 Pair entry if needed
  if (!lbrt_port_is_slave_port(port) && lbrt_port_is_leaf_port(port) &&
      nh->resolved) {
    __u32 vid;
    if ((port->sinfo.port_type & PortReal) != 0) {
      vid = port->port_no + RealPortIDB;
    } else {
      vid = port->port_no + BondIDB;
    }

    lbrt_fdb_key_t fdb_key;
    memset(&fdb_key, 0, sizeof(fdb_key));
    memcpy(fdb_key.mac_addr, nh->attr.hardware_addr, ETH_ALEN);
    fdb_key.bridge_id = vid;

    lbrt_fdb_attr_t fdb_attr;
    memset(&fdb_attr, 0, sizeof(fdb_attr));
    memcpy(fdb_attr.oif, port->name, strlen(port->name));
    parse_ip("0.0.0.0", &fdb_attr.dst);
    fdb_attr.type = FdbPhy;

    ret = lbrt_fdb_add(nhh->zone->l2, &fdb_key, &fdb_attr);
    if (ret < 0 && ret != L2_SAME_FDB_ERR) {
      lbrt_rt_del(nhh->zone->rt, ip_net_str, zone);
      lbrt_neigh_del(nhh, addr, zone);
      flb_log(LOG_LEVEL_ERR, "neigh add - %s:%s mac fail", addr_str, zone);
      return NEIGH_MAC_ERR;
    }
  }

  lbrt_neigh_activate(nhh, nh);

  flb_log(LOG_LEVEL_DEBUG, "neigh added - %s:%s (%lld)", addr_str, zone,
          nh->mark);

  return 0;
}

int lbrt_neigh_del(lbrt_neigh_h_t *nhh, ip_t *addr, const char *zone) {
  char addr_str[IF_ADDRSIZE];
  ip_ntoa(addr, addr_str);

  lbrt_neigh_t *nh = lbrt_neigh_find(nhh, addr_str, zone);
  if (!nh) {
    flb_log(LOG_LEVEL_ERR, "neigh delete - %s:%s doesnt exist", addr_str, zone);
    return NEIGH_NO_ENT_ERR;
  }

  // Delete related L2 Pair entry if needed
  lbrt_port_t *port = nh->o_if_port;
  if (port && !lbrt_port_is_slave_port(port) && lbrt_port_is_leaf_port(port) &&
      nh->resolved) {
    __u32 vid;
    if ((port->sinfo.port_type & PortReal) != 0) {
      vid = port->port_no + RealPortIDB;
    } else {
      vid = port->port_no + BondIDB;
    }

    lbrt_fdb_key_t fdb_key;
    memset(&fdb_key, 0, sizeof(fdb_key));
    memcpy(fdb_key.mac_addr, nh->attr.hardware_addr, ETH_ALEN);
    fdb_key.bridge_id = vid;

    lbrt_fdb_del(nhh->zone->l2, &fdb_key);
  }

  // Delete the host specific to this NH
  ip_net_t ip_net;
  memcpy(&ip_net.ip, addr, sizeof(ip_t));
  if (addr->f.v4) {
    ip_net.mask = 32;
  } else {
    ip_net.mask = 128;
  }
  char ip_net_str[IF_CIDRSIZE];
  ip_net_ntoa(&ip_net, ip_net_str);

  int ret = lbrt_rt_del(nhh->zone->rt, ip_net_str, zone);
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "neigh delete - %s:%s host-rt fail", addr_str, zone);
  }

  lbrt_neigh_datapath(nh, DP_REMOVE);

  if (HASH_COUNT(nh->nh_rt_m) > 0) {
    nh->resolved = false;
    nh->in_active = true;
    memset(nh->attr.hardware_addr, 0, ETH_ALEN);

    lbrt_neigh_activate(nhh, nh);
    flb_log(LOG_LEVEL_DEBUG, "neigh deactivated - %s:%s", addr_str, zone);
    return 0;
  }

  lbrt_neigh_del_all_tun_ep(nhh, nh);
  lbrt_counter_put_counter(nhh->neigh_id, nh->mark);

  nh->t_fdb = NULL;
  nh->mark = COUNTER_OVERFLOW;
  nh->o_if_port = NULL;
  nh->in_active = true;
  nh->resolved = false;

  HASH_DEL(nhh->neigh_map, nh);
  lbrt_neigh_free(nh);
  flb_log(LOG_LEVEL_DEBUG, "neigh deleted - %s:%s", addr_str, zone);

  return 0;
}

int lbrt_neigh_del_by_port(lbrt_neigh_h_t *nhh, const char *port) {
  lbrt_neigh_t *nh, *tmp;
  HASH_ITER(hh, nhh->neigh_map, nh, tmp) {
    if (strcmp(nh->o_if_port->name, port) == 0) {
      lbrt_neigh_del(nhh, &nh->addr, nh->key.zone);
    }
  }
  return 0;
}

void lbrt_neigh_destruct_all(lbrt_neigh_h_t *nhh) {
  ip_t addr;
  lbrt_neigh_t *nh, *tmp;
  HASH_ITER(hh, nhh->neigh_map, nh, tmp) {
    parse_ip(nh->key.nh, &addr);
    lbrt_neigh_del(nhh, &addr, nh->key.zone);
  }
}

int lbrt_neigh_pair_rt(lbrt_neigh_h_t *nhh, lbrt_neigh_t *nh, lbrt_rt_t *rt) {
  lbrt_rt_t *rtm = NULL;
  HASH_FIND(hh, nh->nh_rt_m, &rt->key, sizeof(rt->key), rtm);
  if (rtm) {
    return 1;
  }
  HASH_ADD(hh, nh->nh_rt_m, key, sizeof(rt->key), rt);
  return 0;
}

int lbrt_neigh_un_pair_rt(lbrt_neigh_h_t *nhh, lbrt_neigh_t *nh,
                          lbrt_rt_t *rt) {
  lbrt_rt_t *rtm = NULL;
  HASH_FIND(hh, nh->nh_rt_m, &rt->key, sizeof(rt->key), rtm);
  if (!rtm) {
    return -1;
  }

  HASH_DEL(nh->nh_rt_m, rt);
  if (HASH_COUNT(nh->nh_rt_m) == 0 && nh->in_active) {
    flb_log(LOG_LEVEL_DEBUG, "neigh rt unpair - %s->%s", rt->key.rt_cidr,
            nh->key.nh);
    lbrt_neigh_del(nhh, &nh->addr, nh->key.zone);
    lbrt_neigh_datapath(nh, DP_REMOVE);
  }

  return 0;
}

int lbrt_neigh_tun_ep_datapath(lbrt_neigh_tun_ep_t *tep,
                               enum lbrt_dp_work work) {
  return 0;
}

int lbrt_neigh_datapath(lbrt_neigh_t *nh, enum lbrt_dp_work work) { return 0; }