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
  // TODO
  notifier.port_notifier = NULL;
  lbrt_port_notifier_register(zone->ports, notifier);
  return l2h;
}

void lbrt_l2_h_free(lbrt_l2_h_t *l2h) {
  if (!l2h)
    return;
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
  HASH_FIND(hh, l2h->fdb_map, &key, sizeof(key), fdb);
  return fdb;
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

  // TODO

  HASH_ADD(hh, l2h->fdb_map, fdb_key, sizeof(lbrt_fdb_key_t), fdb);

  lbrt_fdb_datapath(fdb, DP_CREATE);
  flb_log(LOG_LEVEL_DEBUG, "added fdb ent, key[%d, %s]", key->bridge_id,
          mac_ntop(key->mac_addr));

  return 0;
}

int lbrt_fdb_del(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key) {
  lbrt_fdb_t *fdb = NULL;
  HASH_FIND(hh, l2h->fdb_map, key, sizeof(lbrt_fdb_key_t), fdb);
  if (!fdb)
    return L2_NO_FDB_ERR;

  fdb->inactive = true;
  HASH_DEL(l2h->fdb_map, fdb);
  return 0;
}

int lbrt_fdb_datapath(lbrt_fdb_t *fdb, enum lbrt_dp_work work) { return 0; }