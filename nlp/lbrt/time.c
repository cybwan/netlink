#include <lbrt/time.h>
#include <string.h>

void lbrt_time_now(lbrt_time_t *time) {
  memset(time, 0, sizeof(lbrt_time_t));
  clock_gettime(CLOCK_MONOTONIC, &time->monotonic);
  clock_gettime(CLOCK_REALTIME, &time->realtime);
}

__u64 get_clock_sys_time_ns(void) {
  struct timespec tp;
  __u64 time_ns = 0;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  time_ns = (long long)tp.tv_sec * 1000000000 + tp.tv_nsec;
  return time_ns;
}