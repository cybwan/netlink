#ifndef __FLB_LBRT_LAYER2_H__
#define __FLB_LBRT_LAYER2_H__

#include <cmn/types.h>
#include <uthash.h>

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
} lbrt_fdb_tun_attr_t;

typedef struct lbrt_fdb_stat {
  __u64 packets;
  __u64 bytes;
} lbrt_fdb_stat_t;

typedef struct lbrt_fdb_entry {
  lbrt_fdb_key_t fdb_key;
  lbrt_fdb_attr_t fdb_attr;
  lbrt_fdb_tun_attr_t fdb_tun;
  UT_hash_handle hh;
} lbrt_fdb_entry_t;

typedef struct lbrt_l2h {
  lbrt_fdb_entry_t * fdb_map;
} lbrt_l2h_t;

#endif /* __FLB_LBRT_LAYER2_H__ */
