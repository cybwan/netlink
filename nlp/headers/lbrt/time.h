#ifndef __FLB_LBRT_TIME_H__
#define __FLB_LBRT_TIME_H__

#include <linux/types.h>
#include <time.h>

typedef struct lbrt_time {
  struct timespec monotonic;
  struct timespec realtime;
} lbrt_time_t;

void lbrt_time_now(lbrt_time_t *time);

__u64 get_clock_sys_time_ns(void);

#endif /* __FLB_LBRT_TIME_H__ */