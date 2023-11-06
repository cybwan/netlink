#include <lbrt/layer2.h>

extern struct lbrt_net_meta *mh;

lbrt_l2_h_t *lbrt_l2_h_alloc(lbrt_zone_t *zone) {
  lbrt_l2_h_t *l2h;
  l2h = calloc(1, sizeof(*l2h));
  if (!l2h) {
    return NULL;
  }
  l2h->zone = zone;
  // TODO pending
  // z.Ports.PortNotifierRegister(nL2)
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

int lbrt_l2_fdb_add(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key,
                    lbrt_fdb_attr_t *attr) {

  lbrt_fdb_ent_t *fdb = NULL;
  HASH_FIND(hh, l2h->fdb_map, key, sizeof(lbrt_fdb_key_t), fdb);
  if (fdb) {
    if (lbrt_fdb_attr_equal(attr, &fdb->fdb_attr)) {
      return L2_SAME_FDB_ERR;
    }
    lbrt_l2_fdb_del(l2h, key);
  }

  fdb = calloc(1, sizeof(lbrt_fdb_ent_t));
  if (!fdb)
    return L2_ALLOC_ERR;
  memcpy(&fdb->fdb_key, key, sizeof(lbrt_fdb_key_t));
  memcpy(&fdb->fdb_attr, attr, sizeof(lbrt_fdb_attr_t));
  HASH_ADD(hh, l2h->fdb_map, fdb_key, sizeof(lbrt_fdb_key_t), fdb);

  return 0;
}

int lbrt_l2_fdb_del(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key) {
  lbrt_fdb_ent_t *fdb = NULL;
  HASH_FIND(hh, l2h->fdb_map, key, sizeof(lbrt_fdb_key_t), fdb);
  if (!fdb)
    return L2_NO_FDB_ERR;

  fdb->inactive = true;
  HASH_DEL(l2h->fdb_map, fdb);
  return 0;
}