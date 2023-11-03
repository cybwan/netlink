#ifndef __FLB_LBRT_COUNTER_H__
#define __FLB_LBRT_COUNTER_H__

typedef struct lbrt_counter {
  __u64 begin;
  __u64 end;
  __u64 start;
  __u64 len;
  __u64 cap;
  __u64 *counters;
} lbrt_counter_t;

#endif /* __FLB_LBRT_COUNTER_H__ */