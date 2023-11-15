#include <lbrt/mirror.h>

extern struct lbrt_net_meta mh;

UT_icd lbrt_mirr_icd = {sizeof(lbrt_mirr_t), NULL, NULL, NULL};

lbrt_mirr_t *lbrt_mirr_new() {
  lbrt_mirr_t *mirr = calloc(1, sizeof(lbrt_mirr_t));
  utarray_new(mirr->mobjs, &lbrt_mirr_icd);
  return mirr;
}

void lbrt_mirr_free(lbrt_mirr_t *mirr) {
  if (!mirr)
    return;
  utarray_free(mirr->mobjs);
  free(mirr);
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
  lbrt_mirr_t *mirr, *tmp;
  HASH_ITER(hh, mh->mirr_map, mirr, tmp) {
    HASH_DEL(mh->mirr_map, mirr);
    lbrt_mirr_free(mirr);
  }
  free(mh);
}

lbrt_mirr_t *lbrt_mirr_find(lbrt_mirr_h_t *mh, lbrt_mirr_key_t *key) {
  lbrt_mirr_t *mirr = NULL;
  HASH_FIND(hh, mh->mirr_map, key, sizeof(lbrt_mirr_key_t), mirr);
  return mirr;
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