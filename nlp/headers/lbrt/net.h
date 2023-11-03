#ifndef __FLB_LBRT_NET_H__
#define __FLB_LBRT_NET_H__

#include <lbrt/types.h>

typedef struct lbrt_net_meta {
  struct lbrt_zone_h *zn;
  struct lbrt_zone *zr;
  struct lbrt_mutex *mtx;
} lbrt_net_meta_h;

#endif /* __FLB_LBRT_NET_H__ */