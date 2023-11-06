#ifndef __FLB_LBRT_NET_H__
#define __FLB_LBRT_NET_H__

#include <lbrt/types.h>

#define ROOT_ZONE "root"

typedef struct lbrt_net_meta {
  struct lbrt_zone_h *zn;
  struct lbrt_zone *zr;
  struct lbrt_mutex *mtx;
} lbrt_net_meta_h;

void lbrt_net_init(void);
void lbrt_net_uninit(void);

int lbrt_net_port_add(api_port_mod_t *pm);
int lbrt_net_port_del(api_port_mod_t *pm);

#endif /* __FLB_LBRT_NET_H__ */