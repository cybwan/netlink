#include <stdio.h>

#include <lbrt/types.h>

void trie_node_walker(char *b) { flb_log(LOG_LEVEL_INFO, "%s", b); }

void trie_data2str(lbrt_trie_data_t *d, size_t maxlen, char *buf) {
  if (d->f.num) {
    snprintf(buf, maxlen, "%d", d->v.num);
  }
}

void testTire(lbrt_trie_iter_intf_t tf) {
  int ret;
  lbrt_trie_root_t *trieR = lbrt_trie_alloc(false);
  char *route;
  lbrt_trie_data_t data;
  memset(&data, 0, sizeof(data));
  data.f.num = 1;

  route = "192.168.1.1/32";
  data.v.num = 1100;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  route = "192.168.1.0/15";
  data.v.num = 100;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  route = "192.168.1.0/16";
  data.v.num = 99;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "re-added %s:%d", route, data.v.num);
  }

  route = "192.168.1.0/8";
  data.v.num = 1;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  route = "192.168.1.0/16";
  data.v.num = 1;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret == 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d ret=%d", route, data.v.num,
            ret);
  }

  route = "0.0.0.0/0";
  data.v.num = 222;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  route = "8.8.8.8/32";
  data.v.num = 1200;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  route = "10.10.10.10/32";
  data.v.num = 12;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  route = "1.1.1.1/32";
  data.v.num = 1212;
  ret = lbrt_trie_add(trieR, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  lbrt_trie_str(trieR, tf);

  ip_net_t ipnet;
  lbrt_trie_data_t trie_data;
  char cidr[IF_CIDRSIZE];

  ret = lbrt_trie_find(trieR, "192.41.3.1", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "192.0.0.0/8") != 0 || trie_data.v.num != 1) {
    flb_log(LOG_LEVEL_ERR, "failed to find %s", "192.41.3.1");
  }

  ret = lbrt_trie_find(trieR, "195.41.3.1", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "0.0.0.0/0") != 0 || trie_data.v.num != 222) {
    flb_log(LOG_LEVEL_ERR, "failed to find %s", "195.41.3.1");
  }

  ret = lbrt_trie_find(trieR, "8.8.8.8", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "8.8.8.8/32") != 0 || trie_data.v.num != 1200) {
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
  data.v.num = 5100;
  ret = lbrt_trie_add(trieR6, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  route = "2001:db8::1/128";
  data.v.num = 5200;
  ret = lbrt_trie_add(trieR6, route, &data);
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add %s:%d", route, data.v.num);
  }

  ret = lbrt_trie_find(trieR6, "2001:db8::1", &ipnet, &trie_data);
  ip_net_ntoa(&ipnet, cidr);
  if (ret != 0 || strcmp(cidr, "2001:db8::1/128") != 0 ||
      trie_data.v.num != 5200) {
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

int main() {
  lbrt_trie_iter_intf_t tf;
  tf.trie_data2str = trie_data2str;
  tf.trie_node_walker = trie_node_walker;
  testTire(tf);
  flb_log(LOG_LEVEL_INFO, "TRIE DONE!");
  return 0;
}