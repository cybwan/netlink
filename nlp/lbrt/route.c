#include <lbrt/route.h>

extern struct lbrt_net_meta mh;

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
  lbrt_rt_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(&key.rt_cidr, dst, strlen(dst));
  memcpy(&key.zone, zone, strlen(zone));

  if (na_cnt > 1) {
    flb_log(LOG_LEVEL_ERR, "rt add - %s:%s ecmp not supported", dst, zone);
    return RT_NH_ERR;
  }

  lbrt_rt_t *rt = lbrt_rt_find(rh, dst, zone);
  if (rt) {
    flb_log(LOG_LEVEL_ERR, "rt add - %s:%s exists", dst, zone);
    return RT_EXISTS_ERR;
  }

  rt = calloc(1, sizeof(*rt));
  if (!rt)
    return RT_ALLOC_ERR;

  rt->zone_num = rh->zone->zone_num;
  rt->mark = lbrt_counter_get_counter(rh->mark);
  memcpy(&rt->key, &key, sizeof(key));
  if (ra) {
    memcpy(&rt->attr, ra, sizeof(*ra));
  }
  if (na_cnt) {
    rt->nh_attr_cnt = na_cnt;
    rt->nh_attr = calloc(1, sizeof(lbrt_rt_nh_attr_t) * na_cnt);
    for (int i = 0; i < na_cnt; i++) {
      memcpy(&rt->nh_attr[i], &na[i], sizeof(lbrt_rt_nh_attr_t));
    }
  }

  if (na_cnt) {
    rt->tflags |= RT_TYPE_IND;

    if (ra->host_route) {
      rt->tflags |= RT_TYPE_HOST;
    }
  } else {
    rt->tflags |= RT_TYPE_SELF;
  }

  // lbrt_trie_root_t *tr = NULL;
  // ip_net_t net;
  // parse_ip_net(dst, &net);
  // if (net.ip.f.v4) {
  //   tr = rh->trie4;
  // } else {
  //   tr = rh->trie6;
  // }

  HASH_ADD(hh, rh->rt_map, key, sizeof(key), rt);

  lbrt_rt_datapath(rt, DP_CREATE);

  flb_log(LOG_LEVEL_DEBUG, "rt added - %s:%s", dst, zone);

  return 0;
}

int lbrt_rt_del(lbrt_rt_h_t *rh, const char *dst, const char *zone) {
  return 0;
}

int lbrt_rt_datapath(lbrt_rt_t *rt, enum lbrt_dp_work work) { return 0; }