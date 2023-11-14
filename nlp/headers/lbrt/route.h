#ifndef __FLB_LBRT_ROUTE_H__
#define __FLB_LBRT_ROUTE_H__

#include <lbrt/types.h>

#define MaxSysRoutes (32 + 8) * 1024 // 32k Ipv4 + 8k Ipv6

enum {
  RT_TYPE_IND = 0X1,
  RT_TYPE_DYN = 0X2,
  RT_TYPE_SELF = 0X4,
  RT_TYPE_HOST = 0X8,
};

typedef struct lbrt_rt_key {
  char rt_cidr[IF_CIDRSIZE];
  char zone[ZONE_NAMESIZE];
} lbrt_rt_key_t;

typedef struct lbrt_rt_attr {
  __u32 protocol;
  __u32 os_flags;
  bool host_route;
  __u32 ifi;
} lbrt_rt_attr_t;

typedef struct lbrt_rt_nh_attr {
  char nh_addr[IF_ADDRSIZE];
  __u32 link_index;
} lbrt_rt_nh_attr_t;

typedef struct lbrt_rt_stat {
  __u64 packets;
  __u64 bytes;
} lbrt_rt_stat_t;

typedef struct lbrt_rt_dep_obj {
  struct {
    __u8 neigh : 1;
    __u8 fdb : 1;
  } f;
  struct {
    struct lbrt_neigh *neigh;
    struct lbrt_fdb_ent *fdb;
  } v;
} lbrt_rt_dep_obj_t;

typedef struct lbrt_rt {
  struct lbrt_rt_key key;
  char addr[IF_ADDRSIZE];
  __u32 tflags;
  __u32 zone_num;
  __u64 mark;
  enum lbrt_dp_status sync;
  struct lbrt_rt_attr attr;
  struct lbrt_rt_stat stat;
  bool dead;

  UT_array *nh_attr;
  UT_array *next_hops;
  UT_array *rt_dep_objs;

  UT_hash_handle hh;

} lbrt_rt_t;

typedef struct lbrt_rt_h {
  struct lbrt_rt *rt_map;
  struct lbrt_trie_root *trie4;
  struct lbrt_trie_root *trie6;
  struct lbrt_zone *zone;
  struct lbrt_counter *mark;
} lbrt_rt_h_t;

lbrt_rt_h_t *lbrt_rt_h_new(struct lbrt_zone *zone);
void lbrt_rt_h_free(lbrt_rt_h_t *rh);

lbrt_rt_t *lbrt_rt_find(lbrt_rt_h_t *rh, const char *dst, const char *zone);
int lbrt_rt_add(lbrt_rt_h_t *rh, const char *dst, const char *zone,
                lbrt_rt_attr_t *ra, __u16 na_cnt, lbrt_rt_nh_attr_t *na);
int lbrt_rt_del(lbrt_rt_h_t *rh, const char *dst, const char *zone);
int lbrt_rt_del_by_port(lbrt_rt_h_t *rh, const char *port);
int lbrt_rt_destruct_all(lbrt_rt_h_t *rh);

void lbrt_rts_2_str(lbrt_rt_h_t *rh, lbrt_iter_intf_t it);

UT_array *lbrt_route_get(lbrt_rt_h_t *rh);

void lbrt_rt_ticker(lbrt_rt_h_t *rh);

int lbrt_rt_datapath(lbrt_rt_t *rt, enum lbrt_dp_work work);
#endif /* __FLB_LBRT_ROUTE_H__ */