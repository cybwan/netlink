#include <lbrt/qospol.h>

extern struct lbrt_net_meta mh;

UT_icd lbrt_pol_icd = {sizeof(lbrt_pol_t), NULL, NULL, NULL};

lbrt_pol_t *lbrt_pol_new() {
  lbrt_pol_t *p = calloc(1, sizeof(lbrt_pol_t));
  utarray_new(p->pobjs, &lbrt_pol_icd);
  return p;
}

void lbrt_pol_free(lbrt_pol_t *p) {
  if (!p)
    return;
  utarray_free(p->pobjs);
  free(p);
}

lbrt_pol_h_t *lbrt_pol_h_new(lbrt_zone_t *zone) {
  lbrt_pol_h_t *ph;
  ph = calloc(1, sizeof(*ph));
  if (!ph) {
    return NULL;
  }
  ph->zone = zone;
  ph->mark = lbrt_counter_new(1, MaxMirrors);
  if (!ph->mark) {
    lbrt_pol_h_free(ph);
    return NULL;
  }
  return ph;
}

void lbrt_pol_h_free(lbrt_pol_h_t *ph) {
  if (!ph)
    return;
  if (ph->mark) {
    lbrt_counter_free(ph->mark);
    ph->mark = NULL;
  }
  lbrt_pol_t *p, *tmp;
  HASH_ITER(hh, ph->pol_map, p, tmp) {
    HASH_DEL(ph->pol_map, p);
    lbrt_pol_free(p);
  }
  free(ph);
}

lbrt_pol_t *lbrt_pol_find(lbrt_pol_h_t *ph, lbrt_pol_key_t *key) {
  lbrt_pol_t *p = NULL;
  HASH_FIND(hh, ph->pol_map, key, sizeof(lbrt_pol_key_t), p);
  return p;
}

bool lbrt_pol_info_xlate_validate(api_pol_info_t *pinfo) {
  if (pinfo->committed_info_rate < MinPolRate) {
    return false;
  }
  if (pinfo->peak_info_rate < MinPolRate) {
    return false;
  }

  pinfo->committed_info_rate = pinfo->committed_info_rate * 1000000;
  pinfo->peak_info_rate = pinfo->peak_info_rate * 1000000;

  if (pinfo->committed_blk_size == 0) {
    pinfo->committed_blk_size = DflPolBlkSz;
    pinfo->excess_blk_size = 2 * DflPolBlkSz;
  } else {
    pinfo->excess_blk_size = 2 * pinfo->committed_blk_size;
  }
  return true;
}

bool lbrt_pol_obj_validate(api_pol_obj_t *pobj) {
  if (pobj->attachment != PolAttachPort &&
      pobj->attachment != PolAttachLbRule) {
    return false;
  }
  return true;
}
