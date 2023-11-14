#include <lbrt/route.h>

extern struct lbrt_net_meta mh;

UT_icd lbrt_rt_nh_attr_icd = {sizeof(lbrt_rt_nh_attr_t), NULL, NULL, NULL};
UT_icd brt_rt_next_hops_icd = {sizeof(struct lbrt_neight *), NULL, NULL, NULL};
UT_icd brt_rt_dep_objs_icd = {sizeof(lbrt_rt_dep_obj_t), NULL, NULL, NULL};

lbrt_rt_t *lbrt_rt_new() {
  lbrt_rt_t *rt = calloc(1, sizeof(lbrt_rt_t));
  utarray_new(rt->nh_attr, &lbrt_rt_nh_attr_icd);
  utarray_new(rt->next_hops, &brt_rt_next_hops_icd);
  utarray_new(rt->rt_dep_objs, &brt_rt_dep_objs_icd);
  return rt;
}

void lbrt_rt_free(lbrt_rt_t *rt) {
  if (!rt)
    return;
  utarray_free(rt->nh_attr);
  utarray_free(rt->next_hops);
  utarray_free(rt->rt_dep_objs);
  free(rt);
}

lbrt_rt_h_t *lbrt_rt_h_new(lbrt_zone_t *zone) {
  lbrt_rt_h_t *rh;
  rh = calloc(1, sizeof(*rh));
  if (!rh) {
    return NULL;
  }
  rh->zone = zone;
  rh->mark = lbrt_counter_new(1, MaxSysRoutes);
  if (!rh->mark) {
    lbrt_rt_h_free(rh);
    return NULL;
  }
  rh->trie4 = lbrt_trie_alloc(false);
  if (!rh->trie4) {
    lbrt_rt_h_free(rh);
    return NULL;
  }
  rh->trie6 = lbrt_trie_alloc(true);
  if (!rh->trie6) {
    lbrt_rt_h_free(rh);
    return NULL;
  }
  return rh;
}

void lbrt_rt_h_free(lbrt_rt_h_t *rh) {
  if (!rh)
    return;

  if (rh->mark) {
    lbrt_counter_free(rh->mark);
    rh->mark = NULL;
  }

  if (rh->trie4) {
    lbrt_trie_free(rh->trie4);
    rh->trie4 = NULL;
  }

  if (rh->trie6) {
    lbrt_trie_free(rh->trie6);
    rh->trie6 = NULL;
  }

  free(rh);
}

lbrt_rt_t *lbrt_rt_find(lbrt_rt_h_t *rh, const char *dst, const char *zone) {
  lbrt_rt_t *rt = NULL;
  lbrt_rt_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(&key.rt_cidr, dst, strlen(dst));
  memcpy(&key.zone, zone, strlen(zone));
  HASH_FIND(hh, rh->rt_map, &key, sizeof(key), rt);
  return rt;
}

int lbrt_rt_add(lbrt_rt_h_t *rh, const char *dst, const char *zone,
                lbrt_rt_attr_t *ra, __u16 na_cnt, lbrt_rt_nh_attr_t *na) {
  if (na_cnt > 1) {
    flb_log(LOG_LEVEL_ERR, "rt add - %s:%s ecmp not supported", dst, zone);
    return RT_NH_ERR;
  }

  lbrt_rt_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(&key.rt_cidr, dst, strlen(dst));
  memcpy(&key.zone, zone, strlen(zone));

  lbrt_rt_t *rt = lbrt_rt_find(rh, dst, zone);
  if (rt) {
    bool rtMod = false;
    if (utarray_len(rt->nh_attr) != na_cnt) {
      rtMod = true;
    } else {
      lbrt_rt_nh_attr_t *nh_attr = NULL;
      for (__u32 i = 0; i < na_cnt; i++) {
        nh_attr = (lbrt_rt_nh_attr_t *)utarray_eltptr(rt->nh_attr, i);
        if (strcmp(na[i].nh_addr, nh_attr->nh_addr) != 0) {
          rtMod = true;
          break;
        }
      }
    }

    if (rtMod) {
      int ret = lbrt_rt_del(rh, dst, zone);
      if (ret < 0) {
        flb_log(LOG_LEVEL_ERR, "rt add - %s:%s del failed on mod", dst, zone);
        return RT_MOD_ERR;
      }
      return lbrt_rt_add(rh, dst, zone, ra, na_cnt, na);
    }
    flb_log(LOG_LEVEL_ERR, "rt add - %s:%s exists", dst, zone);
    return RT_EXISTS_ERR;
  }

  rt = lbrt_rt_new();
  memcpy(&rt->key, &key, sizeof(key));
  if (ra) {
    memcpy(&rt->attr, ra, sizeof(*ra));
  }
  if (na_cnt) {
    for (int i = 0; i < na_cnt; i++) {
      utarray_push_back(rt->nh_attr, &na[i]);
    }
  }
  rt->zone_num = rh->zone->zone_num;

  if (na_cnt) {
    rt->tflags |= RT_TYPE_IND;

    if (ra->host_route) {
      rt->tflags |= RT_TYPE_HOST;
    }

    // TODO
  } else {
    rt->tflags |= RT_TYPE_SELF;
  }

  lbrt_trie_root_t *tr = NULL;
  ip_net_t net;
  parse_ip_net(dst, &net, NULL);
  if (net.ip.f.v4) {
    tr = rh->trie4;
  } else {
    tr = rh->trie6;
  }

  lbrt_trie_data_t t_data;
  memset(&t_data, 0, sizeof(t_data));
  if (utarray_len(rt->next_hops) > 0) {
    t_data.f.neigh = 1;
    t_data.v.neigh = (lbrt_neigh_t *)utarray_eltptr(rt->next_hops, 0);
  } else {
    t_data.f.osid = 1;
    t_data.v.osid = rt->attr.ifi;
  }
  int tret = lbrt_trie_add(tr, (char *)dst, &t_data);
  if (tret < 0) {
    // TODO
    flb_log(LOG_LEVEL_ERR, "rt add - %s:%s lpm add fail", dst, zone);
    return RT_TRIE_ADD_ERR;
  }

  rt->mark = lbrt_counter_get_counter(rh->mark);

  HASH_ADD(hh, rh->rt_map, key, sizeof(key), rt);

  // TODO

  lbrt_rt_datapath(rt, DP_CREATE);

  flb_log(LOG_LEVEL_DEBUG, "rt added - %s:%s", dst, zone);

  return 0;
}

void __lbrt_rt_clear_deps(lbrt_rt_t *rt) {
  __u32 dep_obj_cnt = utarray_len(rt->rt_dep_objs);
  if (dep_obj_cnt > 0) {
    lbrt_rt_dep_obj_t *dep_obj = NULL;
    for (__u32 i = 0; i < dep_obj_cnt; i++) {
      dep_obj = utarray_eltptr(rt->rt_dep_objs, i);
      if (dep_obj->f.fdb) {
        dep_obj->v.fdb->fdb_tun.rt = NULL;
        dep_obj->v.fdb->fdb_tun.nh = NULL;
        dep_obj->v.fdb->unreach = true;
      } else if (dep_obj->f.neigh) {
        dep_obj->v.neigh->type &= ~NH_RECURSIVE;
        dep_obj->v.neigh->r_mark = 0;
        dep_obj->v.neigh->resolved = false;
      }
    }
  }
}

int lbrt_rt_del(lbrt_rt_h_t *rh, const char *dst, const char *zone) {
  lbrt_rt_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(&key.rt_cidr, dst, strlen(dst));
  memcpy(&key.zone, zone, strlen(zone));

  lbrt_rt_t *rt = lbrt_rt_find(rh, dst, zone);
  if (!rt) {
    flb_log(LOG_LEVEL_ERR, "rt delete - %s:%s not found", dst, zone);
    return RT_NO_ENT_ERR;
  }

  // Take care of any dependencies on this route object
  __lbrt_rt_clear_deps(rt);

  // TODO

  lbrt_trie_root_t *tr = NULL;
  ip_net_t net;
  parse_ip_net(dst, &net, NULL);
  if (net.ip.f.v4) {
    tr = rh->trie4;
  } else {
    tr = rh->trie6;
  }

  int tret = lbrt_trie_del(tr, (char *)dst);
  if (tret < 0) {
    flb_log(LOG_LEVEL_ERR, "rt delete - %s:%s lpm not found", dst, zone);
    return RT_TRIE_DEL_ERR;
  }

  HASH_DELETE(hh, rh->rt_map, rt);

  lbrt_counter_put_counter(rh->mark, rt->mark);

  lbrt_rt_datapath(rt, DP_REMOVE);

  lbrt_rt_free(rt);

  flb_log(LOG_LEVEL_DEBUG, "rt deleted - %s:%s", dst, zone);

  return 0;
}

int lbrt_rt_del_by_port(lbrt_rt_h_t *rh, const char *port) { return 0; }

int lbrt_rt_datapath(lbrt_rt_t *rt, enum lbrt_dp_work work) { return 0; }