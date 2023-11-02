#ifndef __FLB_LBRT_LAYER2_H__
#define __FLB_LBRT_LAYER2_H__

#include <linux/if_ether.h>

typedef struct lbrt_fdb_key {
  __u8 mac_addr[ETH_ALEN];
  __u32 bridge_id;
} lbrt_fdb_key_t;

#endif /* __FLB_LBRT_LAYER2_H__ */
