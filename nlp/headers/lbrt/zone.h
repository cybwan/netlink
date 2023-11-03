#ifndef __FLB_LBRT_ZONE_H__
#define __FLB_LBRT_ZONE_H__

#include <lbrt/types.h>

/* Length of zone name.  */
#define ZONE_NAMESIZE 16

typedef struct lbrt_zone {
  char name[ZONE_NAMESIZE];
  int zone_num;
  struct lbrt_ports_h *ports;
  struct lbrt_vlans_h *vlans;
  struct lbrt_l2_h *l2;
  struct lbrt_neigh_h *nh;
  struct lbrt_rt_h *rt;
  struct lbrt_l3_h *l3;
  struct lbrt_rule_h *rules;
  struct lbrt_sess_h *sess;
  struct lbrt_pol_h *pols;
  struct lbrt_mirr_h *mirrs;
  struct lbrt_mutex *mtx;

  UT_hash_handle hh;

} lbrt_zone_t;

typedef struct lbrt_zone_h {
  lbrt_zone_t *zone_map;
  lbrt_zone_t *zone_brs;
  lbrt_zone_t *zone_ports;
  lbrt_counter_t *zone_mark;
} lbrt_zone_h_t;

struct lbrt_zone_h *lbrt_zone_h_alloc(size_t maxsize);

#endif /* __FLB_LBRT_ZONE_H__ */