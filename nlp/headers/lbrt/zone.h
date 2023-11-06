#ifndef __FLB_LBRT_ZONE_H__
#define __FLB_LBRT_ZONE_H__

#include <lbrt/types.h>

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

typedef struct lbrt_zone_br {
  char br_name[ZONE_NAMESIZE];
  struct lbrt_zone *zone;

  UT_hash_handle hh;

} lbrt_zone_br_t;

typedef struct lbrt_zone_port {
  char port_name[ZONE_NAMESIZE];
  struct lbrt_zone *zone;

  UT_hash_handle hh;

} lbrt_zone_port_t;

typedef struct lbrt_zone_h {
  lbrt_zone_t *zone_map;
  lbrt_zone_br_t *zone_brs;
  lbrt_zone_port_t *zone_ports;
  lbrt_counter_t *zone_mark;
} lbrt_zone_h_t;

lbrt_zone_h_t *lbrt_zone_h_alloc(void);
void lbrt_zone_h_free(lbrt_zone_h_t *zh);

int lbrt_zone_add(lbrt_zone_h_t *zh, const char *name);
lbrt_zone_t *lbrt_zone_find(lbrt_zone_h_t *zh, const char *name);
int lbrt_zone_delete(lbrt_zone_h_t *zh, const char *name);

int lbrt_zone_br_add(lbrt_zone_h_t *zh, const char *br_name,
                     const char *zone_name);
int lbrt_zone_br_delete(lbrt_zone_h_t *zh, const char *br_name);
int lbrt_zone_port_add(lbrt_zone_h_t *zh, const char *port_name,
                       const char *zone_name);
int lbrt_zone_port_delete(lbrt_zone_h_t *zh, const char *port_name);
bool lbrt_zone_port_is_valid(lbrt_zone_h_t *zh, const char *port_name,
                             const char *zone_name);
lbrt_zone_t *lbrt_zone_get_by_port(lbrt_zone_h_t *zh, const char *port_name);

#endif /* __FLB_LBRT_ZONE_H__ */