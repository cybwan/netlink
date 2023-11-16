#include <lbrt/time.h>
#include <stdio.h>

#include <test.h>

void counterTest() {
  __u64 begin = 0;
  __u64 length = 10 ^ 12;
  __u64 flowno;
  struct lbrt_counter *counter = lbrt_counter_new(begin, length);
  for (__u64 i = begin; i < length; i++) {
    flowno = lbrt_counter_get_counter(counter);
    if (flowno == COUNTER_OVERFLOW) {
      flb_log(LOG_LEVEL_ERR, "counter overflow");
    }
  }
  lbrt_counter_free(counter);
}

void counterBenchmark(__u64 loop_cnt) {
  flb_log(LOG_LEVEL_INFO, "Benchmark: %7d", loop_cnt);
  __u64 nano1 = get_clock_sys_time_ns();
  for (__u64 n = 0; n < loop_cnt; n++) {
    counterTest();
  }
  __u64 nano2 = get_clock_sys_time_ns();
  flb_log(LOG_LEVEL_INFO, "Benchmark: %7d %4llu ns/op", loop_cnt,
          (nano2 - nano1) / loop_cnt);
}