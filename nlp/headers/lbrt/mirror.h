#ifndef __FLB_LBRT_MIRROR_H__
#define __FLB_LBRT_MIRROR_H__

#include <lbrt/types.h>

typedef struct lbrt_mirr_key {
  char *name;
} lbrt_mirr_key_t;

typedef struct lbrt_mirr_stats {
  __u64 packets_ok;
  __u64 bytes;
} lbrt_mirr_stats_t;

typedef struct lbrt_mirr_attach_obj {
} lbrt_mirr_attach_obj_t;

typedef struct lbrt_mirr_obj_info {
  lbrt_mirr_attach_obj_t attach_obj;
  enum lbrt_dp_status sync;
} lbrt_mirr_obj_info_t;

typedef struct lbrt_mirr_entry {
  struct lbrt_mirr_key key;
  __u64 hw_num;
  struct lbrt_pol_stats stats;
  enum lbrt_dp_status sync;
  struct lbrt_mirr_obj_info *mobjs;

  UT_hash_handle hh;

} lbrt_mirr_entry_t;

typedef struct lbrt_mirr_h {
  struct lbrt_mirr_entry *mirr_map;
  struct lbrt_zone *zone;
  struct lbrt_counter *mark;
} lbrt_mirr_h_t;

#endif /* __FLB_LBRT_MIRROR_H__ */