#ifndef __FLB_LBRT_ROUTE_H__
#define __FLB_LBRT_ROUTE_H__

#include <lbrt/types.h>

enum {
  RT_ERR_BASE = -5000,
  RT_EXISTS_ERR,
  RT_NH_ERR,
  RT_NOENT_ERR,
  RT_RANGE_ERR,
  RT_MOD_ERR,
  RT_TRIE_ADD_ERR,
  RT_TRIE_DEL_ERR,
};

enum {
  RT_TYPE_IND = 0X1,
  RT_TYPE_DYN = 0X2,
  RT_TYPE_SELF = 0X4,
  RT_TYPE_HOST = 0X8,
};

typedef struct lbrt_rt_key {
  char *rt_cidr;
  char *zone;
} lbrt_rt_key_t;

typedef struct lbrt_rt_attr {
  __u32 protocol;
  __u32 os_flags;
  bool host_route;
  __u32 ifi;
} lbrt_rt_attr_t;

typedef struct lbrt_rt_nh_attr {
  ip_t nh_addr;
  __u32 link_index;
} lbrt_rt_nh_attr_t;

typedef struct lbrt_rt_stat {
  __u64 packets;
  __u64 bytes;
} lbrt_rt_stat_t;

typedef struct lbrt_rt_dep_obj {
} lbrt_rt_dep_obj_t;

typedef struct lbrt_rt {
  struct lbrt_rt_key key;
  ip_t addr;
  struct lbrt_rt_attr attr;
  __u32 tflags;
  bool dead;
  enum lbrt_dp_status sync;
  __u32 zone_num;
  __u64 mark;
  struct lbrt_rt_stat stat;
  struct lbrt_rt_nh_attr *nh_attr;
  struct lbrt_neigh **next_hops;
  struct lbrt_rt_dep_obj *rt_dep_objs;

  UT_hash_handle hh;

} lbrt_rt_t;

typedef struct lbrt_rt_h {
  struct lbrt_rt *rt_map;
  struct lbrt_trie_root *trie4;
  struct lbrt_trie_root *trie6;
  struct lbrt_zone *zone;
  struct lbrt_counter *mark;
} lbrt_rt_h_t;

#endif /* __FLB_LBRT_ROUTE_H__ */