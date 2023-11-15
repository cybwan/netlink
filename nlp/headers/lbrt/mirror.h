#ifndef __FLB_LBRT_MIRROR_H__
#define __FLB_LBRT_MIRROR_H__

#include <lbrt/types.h>

#define MaxMirrors 32

typedef struct lbrt_mirr_key {
  char name[MIRR_NAMESIZE];
} lbrt_mirr_key_t;

typedef struct lbrt_mirr_stats {
  __u64 packets_ok;
  __u64 bytes;
} lbrt_mirr_stats_t;

typedef struct lbrt_mirr_attach_obj {
} lbrt_mirr_attach_obj_t;

typedef struct lbrt_mirr_obj_info {
  api_mirr_obj_t args;
  lbrt_mirr_attach_obj_t attach_obj;
  struct lbrt_mirr_entry *parent;
  enum lbrt_dp_status sync;
} lbrt_mirr_obj_info_t;

typedef struct lbrt_mirr_entry {
  struct lbrt_mirr_key key;
  api_mirr_info_t info;
  struct lbrt_zone *zone;
  __u64 hw_num;
  struct lbrt_pol_stats stats;
  UT_array *mobjs; // struct lbrt_mirr_obj_info
  enum lbrt_dp_status sync;

  UT_hash_handle hh;

} lbrt_mirr_t;

typedef struct lbrt_mirr_h {
  struct lbrt_mirr_entry *mirr_map;
  struct lbrt_zone *zone;
  struct lbrt_counter *mark;
} lbrt_mirr_h_t;

lbrt_mirr_h_t *lbrt_mirr_h_new(struct lbrt_zone *zone);
void lbrt_mirr_h_free(lbrt_mirr_h_t *mh);

lbrt_mirr_t *lbrt_mirr_find(lbrt_mirr_h_t *mh, lbrt_mirr_key_t *key);

bool lbrt_mirr_info_validate(api_mirr_info_t *minfo);
bool lbrt_mirr_obj_validate(api_mirr_obj_t *mobj);
bool lbrt_mirr_info_cmp(api_mirr_info_t *minfo1, api_mirr_info_t *minfo2);

#endif /* __FLB_LBRT_MIRROR_H__ */