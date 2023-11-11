#ifndef __FLB_LBRT_SESSION_H__
#define __FLB_LBRT_SESSION_H__

#include <lbrt/types.h>

typedef struct lbrt_sess_h {
  struct lbrt_zone *zone;
  struct lbrt_counter *mark;
} lbrt_sess_h_t;

#endif /* __FLB_LBRT_SESSION_H__ */