#ifndef __FLB_TEST_H__
#define __FLB_TEST_H__

#include <lbrt/types.h>

void tireTest(struct lbrt_trie_iter_intf tf);
void tireBenchmark(struct lbrt_trie_iter_intf tf, int loop_cnt);
void lbrt_trie_iter_intf_init(struct lbrt_trie_iter_intf *tf);

#endif /* __FLB_TEST_H__ */