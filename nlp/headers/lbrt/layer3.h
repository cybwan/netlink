#ifndef __FLB_LBRT_LAYER3_H__
#define __FLB_LBRT_LAYER3_H__

#include <lbrt/types.h>

typedef struct lbrt_ifa_key {
  char obj[IF_NAMESIZE];
} lbrt_ifa_key_t;

typedef struct lbrt_ifa_ent {
  ip_t ifa_addr;
  ip_net_t ifa_net;
  bool secondary;
} lbrt_ifa_ent_t;

typedef struct lbrt_ifa {
  struct lbrt_ifa_key ifa_key;
  struct lbrt_zone *zone;
  UT_array *ifas; // struct lbrt_ifa_ent
  enum lbrt_dp_status sync;

  UT_hash_handle hh;

} lbrt_ifa_t;

typedef struct lbrt_l3_h {
  struct lbrt_ifa *ifa_map;
  struct lbrt_zone *zone;
} lbrt_l3_h_t;

lbrt_ifa_t *lbrt_ifa_find(lbrt_l3_h_t *l3h, const char *obj);
lbrt_l3_h_t *lbrt_l3_h_new(struct lbrt_zone *zone);
void lbrt_l3_h_free(lbrt_l3_h_t *l3h);

int lbrt_ifa_add(lbrt_l3_h_t *l3h, const char *obj, const char *cidr);
int lbrt_ifa_del(lbrt_l3_h_t *l3h, const char *obj, const char *cidr);
int lbrt_ifa_del_all(lbrt_l3_h_t *l3h, const char *obj);

int lbrt_ifa_select(lbrt_l3_h_t *l3h, const char *obj, ip_t *addr, bool findAny,
                    ip_t *out_sip, char *out_dev);
int lbrt_ifa_select_any(lbrt_l3_h_t *l3h, ip_t *addr, bool findAny,
                        ip_t *out_sip, char *out_dev);

void lbrt_ifa_2_str(lbrt_ifa_t *ifa, lbrt_iter_intf_t it);
void lbrt_ifas_2_str(lbrt_l3_h_t *l3h, lbrt_iter_intf_t it);

int lbrt_ifa_datapath(lbrt_ifa_t *ifa, enum lbrt_dp_work work);

#endif /* __FLB_LBRT_LAYER3_H__ */