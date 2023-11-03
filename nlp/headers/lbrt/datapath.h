#ifndef __FLB_LBRT_DATAPATH_H__
#define __FLB_LBRT_DATAPATH_H__

#include <lbrt/types.h>

typedef enum lbrt_dp_work {
  DP_CREATE = 1,
  DP_REMOVE,
  DP_CHANGE,
  DP_STATSGET,
  DP_STATSCLR,
  DP_MAPGET,
} lbrt_dp_work_t;

typedef enum lbrt_dp_status {
  DP_CREATE_ERR = 1,
  DP_REMOVE_ERR,
  DP_CHANGE_ERR,
  DP_UKNOWN_ERR,
  DP_IN_PROGRESS_ERR,
} lbrt_dp_status_t;

typedef enum lbrt_dp_tun {
  DP_TUN_VXLAN = 1,
  DP_TUN_GRE,
  DP_TUN_GTP,
  DP_TUN_STT,
  DP_TUN_IPIP,
} lbrt_dp_tun_t;

typedef enum lbrt_dp_fw_op {
  DP_FW_DROP = 1,
  DP_FW_FWD,
  DP_FW_RDR,
  DP_FW_TRAP,
} lbrt_dp_fw_op_t;

typedef enum lbrt_dp_nat {
  DP_SNAT = 1,
  DP_DNAT,
  DP_HSNAT,
  DP_HDNAT,
  DP_FULLNAT,
} lbrt_dp_nat_t;

typedef enum lbrt_dp_nat_sel {
  EP_RR = 1,
  EP_HASH,
  EP_PRIO,
} lbrt_dp_nat_sel_t;

typedef enum lbrt_dp_sync_op {
  DP_SYNC_ADD = 1,
  DP_SYNC_DELETE,
  DP_SYNC_GET,
  DP_SYNC_BCAST,
} lbrt_dp_sync_op_t;

#endif /* __FLB_LBRT_DATAPATH_H__ */