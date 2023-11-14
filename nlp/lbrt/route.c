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

int lbrt_rt_del_by_port(lbrt_rt_h_t *rh, const char *port) {
  __u32 nh_cnt = 0;
  UT_array *dsts;
  utarray_new(dsts, &ut_str_icd);
  lbrt_neigh_t *nh = NULL;
  lbrt_rt_t *r, *tmp;
  HASH_ITER(hh, rh->rt_map, r, tmp) {
    if (r->attr.host_route) {
      continue;
    }
    nh_cnt = utarray_len(r->next_hops);
    for (__u32 i = 0; i < nh_cnt; i++) {
      nh = (lbrt_neigh_t *)utarray_eltptr(r->next_hops, i);
      if (strcmp(nh->o_if_port->name, port) == 0) {
        utarray_push_back(dsts, &r->key.rt_cidr);
      }
    }
  }

  char **dst = NULL;
  __u32 dst_cnt = utarray_len(dsts);
  for (__u32 i = 0; i < dst_cnt; i++) {
    dst = (char **)utarray_eltptr(dsts, i);
    lbrt_rt_del(rh, *dst, rh->zone->name);
  }
  utarray_free(dsts);

  return 0;
}

int lbrt_rt_destruct_all(lbrt_rt_h_t *rh) {
  UT_array *dsts;
  utarray_new(dsts, &ut_str_icd);
  lbrt_rt_t *r, *tmp;
  HASH_ITER(hh, rh->rt_map, r, tmp) {
    utarray_push_back(dsts, &r->key.rt_cidr);
  }

  char **dst = NULL;
  __u32 dst_cnt = utarray_len(dsts);
  for (__u32 i = 0; i < dst_cnt; i++) {
    dst = (char **)utarray_eltptr(dsts, i);
    lbrt_rt_del(rh, *dst, rh->zone->name);
  }
  utarray_free(dsts);

  return 0;
}

void __lbrt_rt_2_str(lbrt_rt_t *rt, lbrt_iter_intf_t it) {
  UT_string *s;
  utstring_new(s);

  utstring_printf(s, "%16s ", rt->key.rt_cidr);
  if (utarray_len(rt->nh_attr) > 0) {
    lbrt_rt_nh_attr_t *attr =
        (lbrt_rt_nh_attr_t *)utarray_eltptr(rt->nh_attr, 0);
    utstring_printf(s, "via %12s : ", attr->nh_addr);
  }

  if ((rt->tflags & RT_TYPE_DYN) == RT_TYPE_DYN) {
    utstring_printf(s, "Dyn");
  } else {
    utstring_printf(s, "Static");
  }

  if ((rt->tflags & RT_TYPE_IND) == RT_TYPE_IND) {
    utstring_printf(s, ",In");
  } else {
    utstring_printf(s, ",Dr");
  }

  if ((rt->tflags & RT_TYPE_SELF) == RT_TYPE_SELF) {
    utstring_printf(s, ",Self");
  }

  if ((rt->tflags & RT_TYPE_HOST) == RT_TYPE_HOST) {
    utstring_printf(s, ",Host");
  }

  if (rt->mark > 0) {
    utstring_printf(s, " Mark %lld", rt->mark);
  }

  utstring_printf(s, ",%s", rt->key.zone);

  it.node_walker(utstring_body(s));

  utstring_free(s);
}

void lbrt_rts_2_str(lbrt_rt_h_t *rh, lbrt_iter_intf_t it) {
  lbrt_rt_t *rt, *tmp;
  HASH_ITER(hh, rh->rt_map, rt, tmp) { __lbrt_rt_2_str(rt, it); }
}

static UT_icd api_route_get_icd = {sizeof(api_route_get_t), NULL, NULL, NULL};

UT_array *lbrt_route_get(lbrt_rt_h_t *rh) {
  UT_array *routes;
  utarray_new(routes, &api_route_get_icd);

  UT_string *flags;
  utstring_new(flags);

  lbrt_rt_nh_attr_t *nh_attr = NULL;
  lbrt_rt_t *rt, *rt_tmp = NULL;
  HASH_ITER(hh, rh->rt_map, rt, rt_tmp) {
    api_route_get_t *route = calloc(1, sizeof(api_route_get_t));
    route->hardware_mark = (__u32)rt->mark;
    route->protocol = rt->attr.protocol;
    route->statistic.bytes = (__u32)rt->stat.bytes;
    route->statistic.packets = (__u32)rt->stat.packets;
    route->sync = rt->sync;
    memcpy(route->dst, rt->key.rt_cidr, strlen(rt->key.rt_cidr));
    if (utarray_len(rt->nh_attr) > 0) {
      nh_attr = (lbrt_rt_nh_attr_t *)utarray_eltptr(rt->nh_attr, 0);
      memcpy(route->gw, nh_attr->nh_addr, strlen(nh_attr->nh_addr));
    }

    utstring_clear(flags);
    if ((rt->tflags & RT_TYPE_IND) != 0) {
      utstring_printf(flags, "Ind ");
    }
    if ((rt->tflags & RT_TYPE_DYN) != 0) {
      utstring_printf(flags, "Dyn ");
    }
    if ((rt->tflags & RT_TYPE_SELF) != 0) {
      utstring_printf(flags, "Self ");
    }
    if ((rt->tflags & RT_TYPE_HOST) != 0) {
      utstring_printf(flags, "Host ");
    }
    memcpy(route->flags, utstring_body(flags), utstring_len(flags));

    utarray_push_back(routes, route);
  }

  utstring_free(flags);

  return routes;
}

void lbrt_rt_ticker(lbrt_rt_h_t *rh) {
  // TODO
}

int lbrt_rt_datapath(lbrt_rt_t *rt, enum lbrt_dp_work work) { return 0; }