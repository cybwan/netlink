#include <lbrt/mirror.h>

extern struct lbrt_net_meta mh;

UT_icd lbrt_mirr_icd = {sizeof(lbrt_mirr_t), NULL, NULL, NULL};

lbrt_mirr_t *lbrt_mirr_new() {
  lbrt_mirr_t *m = calloc(1, sizeof(lbrt_mirr_t));
  utarray_new(m->mobjs, &lbrt_mirr_icd);
  return m;
}

void lbrt_mirr_free(lbrt_mirr_t *m) {
  if (!m)
    return;
  utarray_free(m->mobjs);
  free(m);
}

lbrt_mirr_h_t *lbrt_mirr_h_new(lbrt_zone_t *zone) {
  lbrt_mirr_h_t *mh;
  mh = calloc(1, sizeof(*mh));
  if (!mh) {
    return NULL;
  }
  mh->zone = zone;
  mh->mark = lbrt_counter_new(1, MaxMirrors);
  if (!mh->mark) {
    lbrt_mirr_h_free(mh);
    return NULL;
  }
  return mh;
}

void lbrt_mirr_h_free(lbrt_mirr_h_t *mh) {
  if (!mh)
    return;
  if (mh->mark) {
    lbrt_counter_free(mh->mark);
    mh->mark = NULL;
  }
  lbrt_mirr_t *m, *tmp;
  HASH_ITER(hh, mh->mirr_map, m, tmp) {
    HASH_DEL(mh->mirr_map, m);
    lbrt_mirr_free(m);
  }
  free(mh);
}

lbrt_mirr_t *lbrt_mirr_find(lbrt_mirr_h_t *mh, lbrt_mirr_key_t *key) {
  lbrt_mirr_t *m = NULL;
  HASH_FIND(hh, mh->mirr_map, key, sizeof(lbrt_mirr_key_t), m);
  return m;
}

bool lbrt_mirr_info_validate(api_mirr_info_t *minfo) {
  if (minfo->mirr_type != MirrTypeSpan && minfo->mirr_type != MirrTypeRspan &&
      minfo->mirr_type != MirrTypeErspan) {
    return false;
  }
  if (minfo->mirr_type == MirrTypeRspan && minfo->mirr_vlan != 0) {
    return false;
  }
  if (minfo->mirr_type == MirrTypeErspan) {
    ip_t rip, sip;
    parse_ip(minfo->mirr_r_ip, &rip);
    parse_ip(minfo->mirr_s_ip, &sip);
    if ((!rip.f.v4 && !rip.f.v6) || (!sip.f.v4 && !sip.f.v6) ||
        minfo->mirr_tid == 0) {
      return false;
    }
    return false;
  }
  return true;
}

bool lbrt_mirr_obj_validate(api_mirr_obj_t *mobj) {
  if (mobj->attachment != MirrAttachPort &&
      mobj->attachment != MirrAttachRule) {
    return false;
  }
  return true;
}

bool lbrt_mirr_info_cmp(api_mirr_info_t *minfo1, api_mirr_info_t *minfo2) {
  if (minfo1->mirr_type == minfo2->mirr_type &&
      minfo1->mirr_vlan == minfo2->mirr_vlan &&
      minfo1->mirr_tid == minfo2->mirr_tid &&
      strcmp(minfo1->mirr_port, minfo2->mirr_port) == 0 &&
      strcmp(minfo1->mirr_r_ip, minfo2->mirr_r_ip) == 0 &&
      strcmp(minfo1->mirr_s_ip, minfo2->mirr_s_ip) == 0) {
    return true;
  }
  return false;
}

int lbrt_mirr_add(lbrt_mirr_h_t *mh, const char *name, api_mirr_info_t *minfo,
                  api_mirr_obj_t *mobjargs) {
  if (!lbrt_mirr_obj_validate(mobjargs)) {
    flb_log(LOG_LEVEL_ERR, "mirror add - %s: bad attach point", name);
    return MIRR_ATTACH_ERR;
  }

  if (!lbrt_mirr_info_validate(minfo)) {
    flb_log(LOG_LEVEL_ERR, "mirror add - %s: info error", name);
    return MIRR_INFO_ERR;
  }

  lbrt_mirr_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(key.name, name, strlen(name));

  lbrt_mirr_t *m = lbrt_mirr_find(mh, &key);
  if (m) {
    if (!lbrt_mirr_info_cmp(&m->info, minfo)) {
      lbrt_mirr_del(mh, name);
    } else {
      return MIRR_EXISTS_ERR;
    }
  }

  __u64 hw_num = lbrt_counter_get_counter(mh->mark);
  if (hw_num == COUNTER_OVERFLOW) {
    return MIRR_ALLOC_ERR;
  }

  m = lbrt_mirr_new();
  memcpy(m->key.name, name, strlen(name));
  memcpy(&m->info, minfo, sizeof(api_mirr_info_t));
  m->zone = mh->zone;
  m->hw_num = hw_num;

  HASH_ADD(hh, mh->mirr_map, key, sizeof(lbrt_mirr_key_t), m);

  lbrt_mirr_datapath(m, DP_CREATE);

  lbrt_mirr_obj_info_t mobjinfo;
  memset(&mobjinfo, 0, sizeof(mobjinfo));
  memcpy(&mobjinfo.args, mobjargs, sizeof(api_mirr_obj_t));

  lbrt_mirr_obj_info_datapath(&mobjinfo, DP_CREATE);

  utarray_push_back(m->mobjs, &mobjinfo);

  flb_log(LOG_LEVEL_INFO, "mirror added - %s", name);

  return 0;
}

int lbrt_mirr_del(lbrt_mirr_h_t *mh, const char *name) {
  lbrt_mirr_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(key.name, name, strlen(name));

  lbrt_mirr_t *m = lbrt_mirr_find(mh, &key);
  if (!m) {
    flb_log(LOG_LEVEL_ERR, "mirror delete - %s: not found error", name);
    return MIRR_NO_EXIST_ERR;
  }

  lbrt_mirr_obj_info_t *mobjinfo = NULL;
  __u32 mobj_cnt = utarray_len(m->mobjs);
  for (__u32 i = 0; i < mobj_cnt; i++) {
    mobjinfo = (lbrt_mirr_obj_info_t *)utarray_eltptr(m->mobjs, i);
    lbrt_mirr_obj_info_datapath(mobjinfo, DP_REMOVE);
    mobjinfo->parent = NULL;
  }

  lbrt_mirr_datapath(m, DP_REMOVE);
  HASH_DEL(mh->mirr_map, m);
  lbrt_mirr_free(m);

  flb_log(LOG_LEVEL_INFO, "mirror deleted - %s", name);

  return 0;
}

void lbrt_mirr_port_del(lbrt_mirr_h_t *mh, const char *name) {
  __u32 mobj_cnt = 0;
  lbrt_mirr_obj_info_t *mobjinfo = NULL;

  lbrt_mirr_t *m, *tmp;
  HASH_ITER(hh, mh->mirr_map, m, tmp) {
    mobj_cnt = utarray_len(m->mobjs);
    for (__u32 i = 0; i < mobj_cnt; i++) {
      mobjinfo = (lbrt_mirr_obj_info_t *)utarray_eltptr(m->mobjs, i);
      if (mobjinfo->args.attachment == MirrAttachPort &&
          strcmp(mobjinfo->args.mirr_obj_name, name) == 0) {
        mobjinfo->sync = 1;
      }
    }
  }
}

void lbrt_mirr_destruct_all(lbrt_mirr_h_t *mh) {
  lbrt_mirr_t *m, *tmp;
  HASH_ITER(hh, mh->mirr_map, m, tmp) { lbrt_mirr_del(mh, m->key.name); }
}

static UT_icd api_mirr_get_icd = {sizeof(api_mirr_get_t), NULL, NULL, NULL};

UT_array *lbrt_mirr_get(lbrt_mirr_h_t *mh) {
  UT_array *ms;
  utarray_new(ms, &api_mirr_get_icd);

  api_mirr_get_t mg;
  lbrt_mirr_obj_info_t *mobjinfo = NULL;
  lbrt_mirr_t *m, *tmp;
  HASH_ITER(hh, mh->mirr_map, m, tmp) {
    memset(&mg, 0, sizeof(mg));
    memcpy(mg.ident, m->key.name, strlen(m->key.name));
    memcpy(&mg.info, &m->info, sizeof(api_mirr_info_t));
    mg.sync = m->sync;
    mobjinfo = (lbrt_mirr_obj_info_t *)utarray_eltptr(m->mobjs, 0);
    memcpy(&mg.target, &mobjinfo->args, sizeof(api_mirr_obj_t));
    utarray_push_back(ms, &mg);
  }
  return ms;
}

int lbrt_mirr_obj_info_datapath(lbrt_mirr_obj_info_t *mobjinfo,
                                enum lbrt_dp_work work) {
  return 0;
}

int lbrt_mirr_datapath(lbrt_mirr_t *m, enum lbrt_dp_work work) { return 0; }