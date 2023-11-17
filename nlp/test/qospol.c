#include <lbrt/time.h>
#include <stdio.h>

#include <test.h>

extern struct lbrt_net_meta mh;

void polTest(lbrt_pol_h_t *ph) {
  __u64 begin = 0;
  __u64 length = 10 ^ 12;

  api_pol_info_t pinfo;
  api_pol_obj_t pobjargs;

  int ret;

  for (__u64 i = begin; i < length; i++) {
    memset(&pinfo, 0, sizeof(pinfo));
    pinfo.committed_info_rate = 10;
    pinfo.peak_info_rate = 10;
    memset(&pobjargs, 0, sizeof(pobjargs));
    pobjargs.attachment = PolAttachPort;

    ret = lbrt_pol_add(ph, "test", &pinfo, &pobjargs);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "lbrt_pol_add fail, ret=[%d]", ret);
    }

    ret = lbrt_pol_del(ph, "test");
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "lbrt_pol_del fail, ret=[%d]", ret);
    }
  }
}

void polBenchmark(__u64 loop_cnt) {
  lbrt_pol_h_t *ph = lbrt_pol_h_new(mh.zr);
  flb_log(LOG_LEVEL_INFO, "Benchmark: %7d", loop_cnt);
  __u64 nano1 = get_clock_sys_time_ns();
  for (__u64 n = 0; n < loop_cnt; n++) {
    polTest(ph);
  }
  __u64 nano2 = get_clock_sys_time_ns();
  flb_log(LOG_LEVEL_INFO, "Benchmark: %7d %4llu ns/op", loop_cnt,
          (nano2 - nano1) / loop_cnt);
  lbrt_pol_h_free(ph);
}