#include <lbrt/time.h>
#include <string.h>

#define __second_ 1000000000

void lbrt_time_now(lbrt_time_t *time) {
  memset(time, 0, sizeof(lbrt_time_t));
  clock_gettime(CLOCK_MONOTONIC, &time->monotonic);
  clock_gettime(CLOCK_REALTIME, &time->realtime);
}

__s64 lbrt_time_sub(lbrt_time_t *t1, lbrt_time_t *t2) {
  __s64 t1ns = (__s64)t1->monotonic.tv_sec * __second_ + t1->monotonic.tv_nsec;
  __s64 t2ns = (__s64)t2->monotonic.tv_sec * __second_ + t2->monotonic.tv_nsec;
  return t1ns - t2ns;
}

__s64 get_clock_sys_time_ns(void) {
  struct timespec tp;
  __s64 time_ns = 0;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  time_ns = (__s64)tp.tv_sec * __second_ + tp.tv_nsec;
  return time_ns;
}