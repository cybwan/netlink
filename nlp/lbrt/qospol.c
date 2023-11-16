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

int lbrt_pol_add(lbrt_pol_h_t *ph, const char *name, api_pol_info_t *pinfo,
                 api_pol_obj_t *pobjargs) {
  if (!lbrt_pol_obj_validate(pobjargs)) {
    flb_log(LOG_LEVEL_ERR, "policer add - %s: bad attach point", name);
    return POL_ATTACH_ERR;
  }

  if (!lbrt_pol_info_xlate_validate(pinfo)) {
    flb_log(LOG_LEVEL_ERR, "policer add - %s: info error", name);
    return POL_INFO_ERR;
  }

  lbrt_pol_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(key.pol_name, name, strlen(name));

  lbrt_pol_t *p = lbrt_pol_find(ph, &key);
  if (p) {
    if (memcmp(&p->info, pinfo, sizeof(api_pol_info_t)) != 0) {
      lbrt_pol_del(ph, name);
    } else {
      return POL_EXISTS_ERR;
    }
  }

  __u64 hw_num = lbrt_counter_get_counter(ph->mark);
  if (hw_num == COUNTER_OVERFLOW) {
    return MIRR_ALLOC_ERR;
  }

  p = lbrt_pol_new();
  memcpy(p->key.pol_name, name, strlen(name));
  memcpy(&p->info, pinfo, sizeof(api_pol_info_t));
  p->zone = ph->zone;
  p->hw_num = hw_num;

  HASH_ADD(hh, ph->pol_map, key, sizeof(lbrt_pol_key_t), p);

  lbrt_pol_datapath(p, DP_CREATE);

  lbrt_pol_obj_info_t mobjinfo;
  memset(&mobjinfo, 0, sizeof(mobjinfo));
  memcpy(&mobjinfo.args, pobjargs, sizeof(api_pol_obj_t));

  lbrt_pol_obj_info_datapath(&mobjinfo, DP_CREATE);

  utarray_push_back(p->pobjs, &mobjinfo);

  flb_log(LOG_LEVEL_INFO, "policer added - %s", name);

  return 0;
}

int lbrt_pol_del(lbrt_pol_h_t *ph, const char *name) {
  lbrt_pol_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(key.pol_name, name, strlen(name));

  lbrt_pol_t *p = lbrt_pol_find(ph, &key);
  if (!p) {
    flb_log(LOG_LEVEL_ERR, "policer delete - %s: not found error", name);
    return POL_NO_EXIST_ERR;
  }

  lbrt_pol_obj_info_t *pobjinfo = NULL;
  __u32 pobj_cnt = utarray_len(p->pobjs);
  for (__u32 i = 0; i < pobj_cnt; i++) {
    pobjinfo = (lbrt_pol_obj_info_t *)utarray_eltptr(p->pobjs, i);
    lbrt_pol_obj_info_datapath(pobjinfo, DP_REMOVE);
    pobjinfo->parent = NULL;
  }

  lbrt_pol_datapath(p, DP_REMOVE);
  HASH_DEL(ph->pol_map, p);
  lbrt_pol_free(p);

  flb_log(LOG_LEVEL_INFO, "policer deleted - %s", name);

  return 0;
}

void lbrt_pol_port_del(lbrt_pol_h_t *ph, const char *name) {
  __u32 pobj_cnt = 0;
  lbrt_pol_obj_info_t *pobjinfo = NULL;

  lbrt_pol_t *p, *tmp;
  HASH_ITER(hh, ph->pol_map, p, tmp) {
    pobj_cnt = utarray_len(p->pobjs);
    for (__u32 i = 0; i < pobj_cnt; i++) {
      pobjinfo = (lbrt_pol_obj_info_t *)utarray_eltptr(p->pobjs, i);
      if (pobjinfo->args.attachment == PolAttachPort &&
          strcmp(pobjinfo->args.pol_obj_name, name) == 0) {
        pobjinfo->sync = 1;
      }
    }
  }
}

void lbrt_pol_destruct_all(lbrt_pol_h_t *ph) {
  lbrt_pol_t *p, *tmp;
  HASH_ITER(hh, ph->pol_map, p, tmp) { lbrt_pol_del(ph, p->key.pol_name); }
}

static UT_icd api_pol_mod_icd = {sizeof(api_pol_mod_t), NULL, NULL, NULL};

UT_array *lbrt_pol_get(lbrt_pol_h_t *ph) {
  UT_array *ps;
  utarray_new(ps, &api_pol_mod_icd);

  api_pol_mod_t pm;
  lbrt_pol_obj_info_t *pobjinfo = NULL;
  lbrt_pol_t *p, *tmp;
  HASH_ITER(hh, ph->pol_map, p, tmp) {
    memset(&pm, 0, sizeof(pm));
    memcpy(pm.ident, p->key.pol_name, strlen(p->key.pol_name));

    pm.info.color_aware = p->info.color_aware;
    pm.info.peak_info_rate = p->info.peak_info_rate / 1000000;
    pm.info.committed_info_rate = p->info.committed_info_rate / 1000000;
    pm.info.committed_blk_size = p->info.committed_blk_size;
    pm.info.excess_blk_size = p->info.excess_blk_size;
    pm.info.pol_type = p->info.pol_type;

    pobjinfo = (lbrt_pol_obj_info_t *)utarray_eltptr(p->pobjs, 0);
    memcpy(&pm.target, &pobjinfo->args, sizeof(api_pol_obj_t));
    utarray_push_back(ps, &pm);
  }
  return ps;
}

int lbrt_pol_obj_info_datapath(lbrt_pol_obj_info_t *pobjinfo,
                               enum lbrt_dp_work work) {
  return 0;
}

int lbrt_pol_datapath(lbrt_pol_t *p, enum lbrt_dp_work work) { return 0; }