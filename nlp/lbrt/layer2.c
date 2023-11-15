#include <lbrt/layer2.h>

extern struct lbrt_net_meta mh;

lbrt_l2_h_t *lbrt_l2_h_new(lbrt_zone_t *zone) {
  lbrt_l2_h_t *l2h;
  l2h = calloc(1, sizeof(*l2h));
  if (!l2h) {
    return NULL;
  }
  l2h->zone = zone;
  lbrt_port_event_intf_t notifier;
  memset(&notifier, 0, sizeof(notifier));
  notifier.xh = (void *)l2h;
  notifier.port_notifier = lbrt_fdb_port_notifier;
  lbrt_port_notifier_register(zone->ports, notifier);
  return l2h;
}

void lbrt_l2_h_free(lbrt_l2_h_t *l2h) {
  if (!l2h)
    return;
  lbrt_fdb_t *fdb, *tmp;
  HASH_ITER(hh, l2h->fdb_map, fdb, tmp) {
    HASH_DEL(l2h->fdb_map, fdb);
    free(fdb);
  }
  free(l2h);
}

bool lbrt_fdb_attr_equal(lbrt_fdb_attr_t *a1, lbrt_fdb_attr_t *a2) {
  return a1->type == a2->type && memcmp(a1->oif, a2->oif, IF_NAMESIZE) == 0 &&
         memcmp(&a1->dst, &a2->dst, sizeof(ip_t)) == 0;
}

void lbrt_fdb_attr_copy(lbrt_fdb_attr_t *dst, lbrt_fdb_attr_t *src) {
  dst->type = src->type;
  memcpy(dst->oif, src->oif, IF_NAMESIZE);
  memcpy(&dst->dst, &src->dst, sizeof(ip_t));
}

lbrt_fdb_t *lbrt_fdb_find(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key) {
  lbrt_fdb_t *fdb = NULL;
  HASH_FIND(hh, l2h->fdb_map, key, sizeof(lbrt_fdb_key_t), fdb);
  return fdb;
}

int lbrt_fdb_resolve_nh(lbrt_fdb_t *fdb, bool *un_reach) {
  lbrt_port_t *p = fdb->port;
  if (!p) {
    *un_reach = true;
    return L2_VXATTR_ERR;
  }

  lbrt_zone_t *zone = lbrt_zone_find(mh.zn, p->zone);
  if (!zone) {
    *un_reach = true;
    return L2_VXATTR_ERR;
  }

  bool unRch = false;
  lbrt_fdb_attr_t *attr = &fdb->fdb_attr;
  char dst_str[IF_ADDRSIZE];
  ip_ntoa(&attr->dst, dst_str);

  if ((p->sinfo.port_type & PortVxlanBr) == PortVxlanBr) {
    if (attr->type != FdbTun) {
      *un_reach = true;
      return L2_VXATTR_ERR;
    }

    if (!attr->dst.f.v4) {
      *un_reach = true;
      return L2_VXATTR_ERR;
    }

    flb_log(LOG_LEVEL_DEBUG, "fdb tun rt lookup %s", dst_str);

    ip_net_t pDstNet;
    lbrt_trie_data_t tDat;
    int ret = lbrt_trie_find(zone->rt->trie4, dst_str, &pDstNet, &tDat);
    if (ret == 0 && (pDstNet.ip.f.v4 || pDstNet.ip.f.v6)) {
      if (tDat.f.neigh) {
        if (!tDat.v.neigh) {
          *un_reach = true;
          return -1;
        }
      } else {
        *un_reach = true;
        return -1;
      }

      lbrt_neigh_t *nh = tDat.v.neigh;
      if (!nh->in_active) {
        char dst_net_str[IF_CIDRSIZE];
        ip_net_ntoa(&pDstNet, dst_net_str);
        lbrt_rt_t *rt = lbrt_rt_find(zone->rt, dst_net_str, zone->name);
        if (!rt) {
          unRch = true;
          flb_log(LOG_LEVEL_DEBUG, "fdb tun rtlookup %s no-rt", dst_str);
        } else {
          // TODO
        }
      }
    } else {
      unRch = true;
      flb_log(LOG_LEVEL_DEBUG, "fdb tun rtlookup %s no trie-ent", dst_str);
    }
  }

  if (unRch) {
    flb_log(LOG_LEVEL_DEBUG, "fdb tun rtlookup %s unreachable", dst_str);
  }

  *un_reach = unRch;
  return 0;
}

int lbrt_fdb_add(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key, lbrt_fdb_attr_t *attr) {
  lbrt_port_t *p = lbrt_port_find_by_name(l2h->zone->ports, attr->oif);
  if (!p || !p->sinfo.port_active) {
    flb_log(LOG_LEVEL_DEBUG, "fdb port not found %s", attr->oif);
    return L2_OIF_ERR;
  }

  lbrt_fdb_t *fdb = lbrt_fdb_find(l2h, key);
  if (fdb) {
    if (lbrt_fdb_attr_equal(attr, &fdb->fdb_attr)) {
      flb_log(LOG_LEVEL_DEBUG, "fdb ent exists, key[%d, %s]", key->bridge_id,
              mac_ntop(key->mac_addr));
      return L2_SAME_FDB_ERR;
    }
    lbrt_fdb_del(l2h, key);
  }

  fdb = calloc(1, sizeof(lbrt_fdb_t));
  if (!fdb)
    return L2_ALLOC_ERR;
  fdb->port = p;
  memcpy(&fdb->fdb_key, key, sizeof(lbrt_fdb_key_t));
  memcpy(&fdb->fdb_attr, attr, sizeof(lbrt_fdb_attr_t));
  lbrt_time_now(&fdb->itime);
  lbrt_time_now(&fdb->stime);

  if ((p->sinfo.port_type & PortVxlanBr) == PortVxlanBr) {
    bool unRch;
    int ret = lbrt_fdb_resolve_nh(fdb, &unRch);
    if (ret < 0) {
      flb_log(LOG_LEVEL_DEBUG, "tun-fdb ent resolve error,key[%d, %s]",
              key->bridge_id, mac_ntop(key->mac_addr));
      return ret;
    }
    fdb->unreach = unRch;
  }

  HASH_ADD(hh, l2h->fdb_map, fdb_key, sizeof(lbrt_fdb_key_t), fdb);

  lbrt_fdb_datapath(fdb, DP_CREATE);
  flb_log(LOG_LEVEL_DEBUG, "added fdb ent, key[%d, %s]", key->bridge_id,
          mac_ntop(key->mac_addr));

  return 0;
}

int lbrt_fdb_del(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key) {
  lbrt_fdb_t *fdb = lbrt_fdb_find(l2h, key);
  if (!fdb) {
    flb_log(LOG_LEVEL_DEBUG, "fdb ent not found, key[%d, %s]", key->bridge_id,
            mac_ntop(key->mac_addr));
    return L2_NO_FDB_ERR;
  }

  if (fdb->port->sinfo.port_type == PortVxlanBr) {
    if (fdb->fdb_tun.rt) {
      lbrt_rt_t *rt = fdb->fdb_tun.rt;
      __u32 dep_obj_cnt = utarray_len(rt->rt_dep_objs);
      lbrt_rt_dep_obj_t *dep_obj = NULL;
      for (__u32 i = 0; i < dep_obj_cnt; i++) {
        dep_obj = (lbrt_rt_dep_obj_t *)utarray_eltptr(rt->rt_dep_objs, i);
        if (dep_obj->f.fdb) {
          if ((lbrt_fdb_t *)dep_obj->v.fdb == fdb) {
            utarray_erase(rt->rt_dep_objs, i, 1);
            break;
          }
        }
      }
    }

    fdb->fdb_tun.rt = NULL;
    if (fdb->fdb_tun.nh) {
      fdb->fdb_tun.nh->resolved = false;
      fdb->fdb_tun.nh = NULL;
    }
    fdb->fdb_tun.ep = NULL;
  }

  lbrt_fdb_datapath(fdb, DP_REMOVE);

  fdb->inactive = true;

  HASH_DEL(l2h->fdb_map, fdb);

  flb_log(LOG_LEVEL_DEBUG, "deleted fdb ent, key[%d, %s]", key->bridge_id,
          mac_ntop(key->mac_addr));

  return 0;
}

void lbrt_fdb_destruct_all(lbrt_l2_h_t *l2h) {
  lbrt_fdb_t *f, *tmp;
  HASH_ITER(hh, l2h->fdb_map, f, tmp) { lbrt_fdb_del(l2h, &f->fdb_key); }
}

void lbrt_fdb_port_notifier(void *xh, const char *name, int osid,
                            __u8 ev_type) {
  lbrt_l2_h_t *l2h = (lbrt_l2_h_t *)xh;
  if ((ev_type & (PortEvDown | PortEvDelete | PortEvLowerDown)) != 0) {
    lbrt_fdb_t *f, *tmp;
    HASH_ITER(hh, l2h->fdb_map, f, tmp) {
      if (strcmp(f->fdb_attr.oif, name) == 0) {
        lbrt_fdb_del(l2h, &f->fdb_key);
      }
    }
  }
}

void __lbrt_fdb_2_str(lbrt_fdb_t *fdb, lbrt_iter_intf_t it, int n) {
  UT_string *s;
  utstring_new(s);

  utstring_printf(
      s, "FdbEnt%-3d : ether %02x:%02x:%02x:%02x:%02x:%02x,br %d :: Oif %s", n,
      fdb->fdb_key.mac_addr[0], fdb->fdb_key.mac_addr[1],
      fdb->fdb_key.mac_addr[2], fdb->fdb_key.mac_addr[3],
      fdb->fdb_key.mac_addr[4], fdb->fdb_key.mac_addr[5],
      fdb->fdb_key.bridge_id, fdb->fdb_attr.oif);

  it.node_walker(utstring_body(s));

  utstring_free(s);
}

void lbrt_fdbs_2_str(lbrt_l2_h_t *l2h, lbrt_iter_intf_t it) {
  int n = 1;
  lbrt_fdb_t *f, *tmp;
  HASH_ITER(hh, l2h->fdb_map, f, tmp) {
    __lbrt_fdb_2_str(f, it, n);
    n++;
  }
}

void lbrt_fdb_ticker(lbrt_l2_h_t *l2h) {
  // TODO
}

int lbrt_fdb_datapath(lbrt_fdb_t *fdb, enum lbrt_dp_work work) { return 0; }