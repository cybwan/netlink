#ifndef __FLB_LBRT_TRIE_H__
#define __FLB_LBRT_TRIE_H__

#include <lbrt/types.h>

#define TrieJmpLength 8
#define PrefixArrLenfth ((1 << (TrieJmpLength + 1)) - 1)
#define PrefixArrNbits                                                         \
  (((PrefixArrLenfth + TrieJmpLength) & ~TrieJmpLength) / TrieJmpLength)
#define PtrArrLength (1 << TrieJmpLength)
#define PtrArrNBits                                                            \
  (((PtrArrLength + TrieJmpLength) & ~TrieJmpLength) / TrieJmpLength)

typedef struct lbrt_trie_data {
  struct {
    __u8 ptr : 1;
    __u8 num : 1;
  } f;
  struct {
    void *ptr;
    __u32 num;
  } v;
} lbrt_trie_data_t;

typedef struct lbrt_trie_iter_intf {
  void (*trie_node_walker)(char *);
  char *(*trie_data2str)(lbrt_trie_data_t *);
} lbrt_trie_iter_intf_t;

typedef struct lbrt_trie_root {
  bool v6;
  __u8 prefixArr[PrefixArrNbits];
  __u8 ptrArr[PtrArrNBits];
  struct lbrt_trie_data *prefixData[PrefixArrLenfth];
  struct lbrt_trie_root *ptrData[PtrArrLength];
} lbrt_trie_root_t;

lbrt_trie_root_t *lbrt_trie_alloc(bool v6);
void lbrt_trie_free(lbrt_trie_root_t *root);

int AddTrie(lbrt_trie_root_t *t, char *cidr, lbrt_trie_data_t *data);
int DelTrie(lbrt_trie_root_t *t, char *cidr);
int FindTrie(lbrt_trie_root_t *t, char *ip, ip_net_t *ipnet,
             lbrt_trie_data_t *trieData);

void Trie2String(lbrt_trie_root_t *t, lbrt_trie_iter_intf_t tf);
#endif /* __FLB_LBRT_TRIE_H__ */