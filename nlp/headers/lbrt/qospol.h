#ifndef __FLB_LBRT_QOSPOL_H__
#define __FLB_LBRT_QOSPOL_H__

#include <lbrt/types.h>

typedef struct lbrt_pol_stats {
  __u64 packets_ok;
  __u64 packets_nok;
  __u64 bytes;
} lbrt_pol_stats_t;

typedef struct lbrt_pol_h {
} lbrt_pol_h_t;

#endif /* __FLB_LBRT_QOSPOL_H__ */