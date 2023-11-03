#ifndef __FLB_LBRT_LAYER2_H__
#define __FLB_LBRT_LAYER2_H__

#include <lbrt/types.h>

typedef struct lbrt_fdb_key {
  __u8 mac_addr[ETH_ALEN];
  __u32 bridge_id;
} lbrt_fdb_key_t;

typedef struct lbrt_fdb_attr {
  char oif[IF_NAMESIZE];
  ip_t dst;
  __u32 type;
} lbrt_fdb_attr_t;

typedef struct lbrt_fdb_tun_attr {
  struct lbrt_rt *rt;
  struct lbrt_neigh *nh;
  struct lbrt_neigh_tun_ep *ep;
} lbrt_fdb_tun_attr_t;

typedef struct lbrt_fdb_stat {
  __u64 packets;
  __u64 bytes;
} lbrt_fdb_stat_t;

typedef struct lbrt_fdb_ent {
  struct lbrt_fdb_key fdb_key;
  struct lbrt_fdb_attr fdb_attr;
  struct lbrt_fdb_tun_attr fdb_tun;
  struct lbrt_port *port;
  struct lbrt_time itime;
  struct lbrt_time stime;
  bool unreach;
  bool inactive;
  enum lbrt_dp_status sync;

  UT_hash_handle hh;

} lbrt_fdb_ent_t;

typedef struct lbrt_l2_h {
  struct lbrt_fdb_ent *fdb_map;
} lbrt_l2_h_t;

#endif /* __FLB_LBRT_LAYER2_H__ */
