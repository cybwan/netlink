#ifndef __FLB_LBRT_QOSPOL_H__
#define __FLB_LBRT_QOSPOL_H__

#include <lbrt/types.h>

typedef struct lbrt_pol_key {
  char *pol_name;
} lbrt_pol_key_t;

typedef struct lbrt_pol_obj_info {

} lbrt_pol_obj_info_t;

typedef struct lbrt_pol_attach_obj {
  // Args      cmn.PolObj
  struct lbrt_pol_obj_info attach_obj;
  struct lbrt_pol_entry *parent;
  enum lbrt_dp_status sync;
} lbrt_pol_attach_obj_t;

typedef struct lbrt_pol_entry {
  struct lbrt_pol_key key;
  // Info  cmn.PolInfo
  struct lbrt_zone *zone;
  __u64 hw_num;
  struct lbrt_pol_stats stats;
  enum lbrt_dp_status Sync;
  struct lbrt_pol_obj_info *pobjs;

  UT_hash_handle hh;

} lbrt_pol_entry_t;

typedef struct lbrt_pol_h {
  struct lbrt_pol_entry *pol_map;
  struct lbrt_zone *zone;
  struct lbrt_counter *mark;
} lbrt_pol_h_t;

#endif /* __FLB_LBRT_QOSPOL_H__ */