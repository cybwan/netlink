#ifndef __FLB_LBRT_COUNTER_H__
#define __FLB_LBRT_COUNTER_H__

#include <linux/types.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct lbrt_counter {
  __u64 begin;
  __u64 end;
  __u64 start;
  __u64 len;
  __u64 cap;
  __u64 *counters;
} lbrt_counter_t;

struct lbrt_counter *lbrt_counter_alloc(__u64 begin, __u64 length);
void lbrt_counter_free(struct lbrt_counter *c);

__u64 lbrt_counter_get_counter(struct lbrt_counter *c);
bool lbrt_counter_put_counter(struct lbrt_counter *c, __u64 id);

#endif /* __FLB_LBRT_COUNTER_H__ */