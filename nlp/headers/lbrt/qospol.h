#ifndef __FLB_LBRT_QOSPOL_H__
#define __FLB_LBRT_QOSPOL_H__

#include <lbrt/types.h>

#define MinPolRate 8
#define MaxPols 8 * 1024
#define DflPolBlkSz 6 * 5000 * 1000

typedef struct lbrt_pol_key {
  char pol_name[POL_NAMESIZE];
} lbrt_pol_key_t;

typedef struct lbrt_pol_obj_info {
  api_pol_obj_t args;
  struct lbrt_pol *parent;
  enum lbrt_dp_status sync;
} lbrt_pol_obj_info_t;

typedef struct lbrt_pol {
  struct lbrt_pol_key key;
  api_pol_info_t info;
  struct lbrt_zone *zone;
  __u64 hw_num;
  struct lbrt_pol_stats stats;
  enum lbrt_dp_status Sync;
  UT_array *pobjs; // struct lbrt_pol_obj_info

  UT_hash_handle hh;

} lbrt_pol_t;

typedef struct lbrt_pol_h {
  struct lbrt_pol *pol_map;
  struct lbrt_zone *zone;
  struct lbrt_counter *mark;
} lbrt_pol_h_t;

lbrt_pol_h_t *lbrt_pol_h_new(struct lbrt_zone *zone);
void lbrt_pol_h_free(lbrt_pol_h_t *ph);

lbrt_pol_t *lbrt_pol_find(lbrt_pol_h_t *ph, lbrt_pol_key_t *key);

bool lbrt_pol_info_xlate_validate(api_pol_info_t *pinfo);
bool lbrt_pol_obj_validate(api_pol_obj_t *pobj);

int lbrt_pol_add(lbrt_pol_h_t *ph, const char *name, api_pol_info_t *pinfo,
                 api_pol_obj_t *pobjargs);
int lbrt_pol_del(lbrt_pol_h_t *ph, const char *name);

int lbrt_pol_obj_info_datapath(lbrt_pol_obj_info_t *pobjinfo,
                               enum lbrt_dp_work work);
int lbrt_pol_datapath(lbrt_pol_t *p, enum lbrt_dp_work work);

#endif /* __FLB_LBRT_QOSPOL_H__ */