#include <stdio.h>

#include <lbrt/types.h>

void trie_node_walker(char *b) { flb_log(LOG_LEVEL_INFO, "%s", b); }

void trie_data2str(lbrt_trie_data_t *d, size_t maxlen, char *buf) {
  if (d->f.num) {
    snprintf(buf, maxlen, "%d", d->v.num);
  }
}

void testTire(void) {
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

  lbrt_trie_iter_intf_t tf;
  tf.trie_data2str = trie_data2str;
  tf.trie_node_walker = trie_node_walker;
  lbrt_trie_str(trieR, tf);
}

int main() {
  testTire();
  flb_log(LOG_LEVEL_INFO, "TRIE DONE!");
  return 0;
}