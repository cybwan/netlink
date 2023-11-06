#ifndef __FLB_LBRT_NEIGH_H__
#define __FLB_LBRT_NEIGH_H__

#include <lbrt/types.h>

typedef struct lbrt_neigh_key {
  char *nh;
  char *zone;
} lbrt_neigh_key_t;

typedef struct lbrt_neigh_attr {
  __u32 os_link_index;
  __u32 os_state;
  __u8 hardware_addr[ETH_ALEN];
} lbrt_neigh_attr_t;

typedef enum lbrt_nh_type {
  NH_NORMAL = 1,
  NH_TUN,
  NH_RECURSIVE,
} lbrt_nh_type_t;

typedef struct lbrt_neigh_tun_ep {
  ip_t s_ip;
  ip_t r_ip;
  __u32 tun_id;
  enum lbrt_dp_tun tun_type;
  __u64 mark;
  struct lbrt_neigh *parent;
  bool in_active;
  enum lbrt_dp_status sync;
} lbrt_neigh_tun_ep_t;

typedef struct lbrt_neigh {
  struct lbrt_neigh_key key;
  ip_t addr;
  struct lbrt_neigh_attr attr;
  bool in_active;
  bool resolved;
  __u64 mark;
  __u64 r_mark;
  struct lbrt_neigh *rec_nh;
  struct lbrt_fdb_ent *t_fdb;
  struct lbrt_neigh_tun_ep **tun_eps;
  enum lbrt_nh_type type;
  enum lbrt_dp_status sync;
  struct lbrt_port *o_if_port;
  struct lbrt_time ats;
  struct lbrt_rt *nh_rt_m;

  UT_hash_handle hh;

} lbrt_neigh_t;

typedef struct lbrt_neigh_h {
  struct lbrt_neigh *neigh_map;
  struct lbrt_counter *neigh_id;
  struct lbrt_counter *neigh_tid;
  struct lbrt_zone *zone;
} lbrt_neigh_h_t;

#endif /* __FLB_LBRT_NEIGH_H__ */