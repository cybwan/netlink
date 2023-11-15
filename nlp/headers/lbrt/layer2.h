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

} lbrt_fdb_t;

typedef struct lbrt_l2_h {
  struct lbrt_fdb_ent *fdb_map;
  struct lbrt_zone *zone;
} lbrt_l2_h_t;

lbrt_l2_h_t *lbrt_l2_h_new(struct lbrt_zone *zone);
void lbrt_l2_h_free(lbrt_l2_h_t *l2h);

bool lbrt_fdb_attr_equal(lbrt_fdb_attr_t *a1, lbrt_fdb_attr_t *a2);
void lbrt_fdb_attr_copy(lbrt_fdb_attr_t *dst, lbrt_fdb_attr_t *src);

lbrt_fdb_t *lbrt_fdb_find(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key);

int lbrt_fdb_add(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key, lbrt_fdb_attr_t *attr);
int lbrt_fdb_del(lbrt_l2_h_t *l2h, lbrt_fdb_key_t *key);
void lbrt_fdb_destruct_all(lbrt_l2_h_t *l2h);

void lbrt_fdbs_2_str(lbrt_l2_h_t *l2h, lbrt_iter_intf_t it);

void lbrt_fdb_port_notifier(void *xh, const char *name, int osid, __u8 ev_type);

void lbrt_fdb_ticker(lbrt_l2_h_t *l2h);

int lbrt_fdb_datapath(lbrt_fdb_t *fdb, enum lbrt_dp_work work);

#endif /* __FLB_LBRT_LAYER2_H__ */
