#ifndef __FLB_LBRT_LAYER3_H__
#define __FLB_LBRT_LAYER3_H__

#include <lbrt/types.h>

typedef struct lbrt_ifa_key {
  char *obj;
} lbrt_ifa_key_t;

typedef struct lbrt_ifa_ent {
  ip_t ifa_addr;
  ip_net_t ifa_net;
  bool secondary;
} lbrt_ifa_entry_t;

typedef struct lbrt_ifa {
  struct lbrt_ifa_key ifa_key;
  struct lbrt_zone *zone;
  enum lbrt_dp_status sync;
  struct lbrt_ifa_ent **ifas;

  UT_hash_handle hh;

} lbrt_ifa_t;

typedef struct lbrt_l3_h {
  struct lbrt_ifa *ifa_map;
  struct lbrt_zone *zone;
} lbrt_l3_h_t;

#endif /* __FLB_LBRT_LAYER3_H__ */