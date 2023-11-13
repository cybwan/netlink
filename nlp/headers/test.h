#ifndef __FLB_TEST_H__
#define __FLB_TEST_H__

#include <lbrt/types.h>

void tireTest(struct lbrt_trie_iter_intf tf);
void tireBenchmark(struct lbrt_trie_iter_intf tf, int loop_cnt);
int test_trie_main(void);

int test_port_main(void);
int test_l2_main(void);
int test_l3_main(void);

static void node_walker(char *b) { flb_log(LOG_LEVEL_DEBUG, "%s", b); }

static inline void lbrt_iter_intf_init(lbrt_iter_intf_t *tf) {
  tf->node_walker = node_walker;
}

static void trie_data2str(lbrt_trie_data_t *d, size_t maxlen, char *buf) {
  if (d->f.osid) {
    snprintf(buf, maxlen, "%d", d->v.osid);
  }
}

static inline void lbrt_trie_iter_intf_init(lbrt_trie_iter_intf_t *tf) {
  tf->trie_data2str = trie_data2str;
  tf->trie_node_walker = node_walker;
}

#endif /* __FLB_TEST_H__ */