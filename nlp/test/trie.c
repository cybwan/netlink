#include <lbrt/time.h>
#include <stdio.h>

#include <test.h>

void tireBenchmark(lbrt_trie_iter_intf_t tf, int loop_cnt) {
  int ret;

  lbrt_trie_root_t *trieR = lbrt_trie_alloc(false);

  lbrt_trie_data_t data;
  memset(&data, 0, sizeof(data));
  data.f.osid = 1;

  int i = 0;
  int j = 0;
  int k = 0;
  int pLen = 32;

  char route[IF_CIDRSIZE];

  __u64 nano1 = get_clock_sys_time_ns();

  for (int n = 0; n < loop_cnt; n++) {
    i = n & 0xff;
    j = n >> 8 & 0xff;
    k = n >> 16 & 0xff;

    memset(route, 0, IF_CIDRSIZE);
    snprintf(route, IF_CIDRSIZE, "192.%d.%d.%d/%d", k, j, i, pLen);
    data.v.osid = n;
    ret = lbrt_trie_add(trieR, route, &data);
    if (ret != 0) {
      flb_log(LOG_LEVEL_ERR, "failed to add %s:%d - (%d)", route, n, ret);
      // lbrt_trie_str(trieR, tf);
    }
  }

  __u64 nano2 = get_clock_sys_time_ns();
  printf("Benchmark: %7d %4llu ns/op\n", loop_cnt, (nano2 - nano1) / loop_cnt);
}

void tireTest(lbrt_trie_iter_intf_t tf) {
  int ret;
  lbrt_trie_root_t *trieR = lbrt_trie_alloc(false);
  char *route;
  lbrt_trie_data_t data;
  memset(&data, 0, sizeof(data));
  data.f.osid = 1;

  route = "192.168.1.1/32";
  data.v.osid = 1100;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  route = "192.168.1.0/15";
  data.v.osid = 100;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  route = "192.168.1.0/16";
  data.v.osid = 99;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "re-added %s:%d", route, data.v.osid);
  }

  route = "192.168.1.0/8";
  data.v.osid = 1;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  route = "192.168.1.0/16";
  data.v.osid = 1;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret == 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d ret=%d", route, data.v.osid,
            ret);
  }

  route = "0.0.0.0/0";
  data.v.osid = 222;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  route = "8.8.8.8/32";
  data.v.osid = 1200;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  route = "10.10.10.10/32";
  data.v.osid = 12;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  route = "1.1.1.1/32";
  data.v.osid = 1212;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  lbrt_trie_str(trieR, tf);

  ip_net_t ipnet;
  lbrt_trie_data_t trie_data;
  char cidr[IF_CIDRSIZE];

  ret = lbrt_trie_find(trieR, "192.41.3.1", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "192.0.0.0/8") != 0 || trie_data.v.osid != 1) {
    flb_log(LOG_LEVEL_ERR, "failed to find %s", "192.41.3.1");
  }

  ret = lbrt_trie_find(trieR, "195.41.3.1", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "0.0.0.0/0") != 0 || trie_data.v.osid != 222) {
    flb_log(LOG_LEVEL_ERR, "failed to find %s", "195.41.3.1");
  }

  ret = lbrt_trie_find(trieR, "8.8.8.8", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "8.8.8.8/32") != 0 || trie_data.v.osid != 1200) {
    flb_log(LOG_LEVEL_ERR, "failed to find %s", "8.8.8.8");
  }

  route = "0.0.0.0/0";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  flb_log(LOG_LEVEL_INFO, "TRIE4 LEFT");
  lbrt_trie_str(trieR, tf);

  ret = lbrt_trie_find(trieR, "195.41.3.1", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret == 0) {
    flb_log(LOG_LEVEL_ERR, "failed to find %s", "195.41.3.1");
  }

  route = "192.168.1.1/32";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "192.168.1.0/15";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "192.168.1.0/16";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "192.168.1.0/8";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "0.0.0.0/0";
  ret = lbrt_trie_del(trieR, route);
  if (ret == 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "8.8.8.8/32";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "10.10.10.10/32";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "1.1.1.1/24";
  ret = lbrt_trie_del(trieR, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  flb_log(LOG_LEVEL_INFO, "TRIE4 LEFT");
  lbrt_trie_str(trieR, tf);

  lbrt_trie_root_t *trieR6 = lbrt_trie_alloc(true);

  route = "2001:db8::/32";
  data.v.osid = 5100;
  ret = lbrt_trie_add(trieR6, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  route = "2001:db8::1/128";
  data.v.osid = 5200;
  ret = lbrt_trie_add(trieR6, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.osid);
  }

  ret = lbrt_trie_find(trieR6, "2001:db8::1", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "2001:db8::1/128") != 0 ||
      trie_data.v.osid != 5200) {
    flb_log(LOG_LEVEL_ERR, "failed to find %s", "2001:db8::1");
  }

  route = "2001:db8::1/128";
  ret = lbrt_trie_del(trieR6, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  route = "2001:db8::/32";
  ret = lbrt_trie_del(trieR6, route);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to delete %s", route);
  }

  flb_log(LOG_LEVEL_INFO, "TRIE6 LEFT");
  lbrt_trie_str(trieR6, tf);
}

int test_trie_main(void) {
  lbrt_trie_iter_intf_t tf;
  lbrt_trie_iter_intf_init(&tf);
  tireTest(tf);
  tireBenchmark(tf, 1);
  tireBenchmark(tf, 100);
  tireBenchmark(tf, 10000);
  tireBenchmark(tf, 948698);
  tireBenchmark(tf, 1218399);
  flb_log(LOG_LEVEL_INFO, "TRIE DONE!");
  return 0;
}