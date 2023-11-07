#ifndef __FLB_API_DEFS_H__
#define __FLB_API_DEFS_H__

#include <cmn/types.h>

enum {
  PortReal = 1, // Base port type
  PortBondSif,  // Bond slave port type
  PortBond,     // Bond port type
  PortVlanSif,  // Vlan slave port type
  PortVlanBr,   // Vlan Br port type
  PortVxlanSif, // Vxlan slave port type
  PortVxlanBr,  // Vxlan br port type
  PortWg,       // Wireguard port type
  PortVti,      // Vti port type
  PortIPTun,    // IPInIP port type
  PortGre,      // GRE port type
};

enum PortProp {
  PortPropUpp = 1, // User-plane processing enabled
  PortPropSpan,    //  SPAN is enabled
  PortPropPol,     //  -Policer is active
};

typedef __u8 DpStatusT;

typedef struct api_port_mod {
  char dev[IF_NAMESIZE];          // name of port
  __u32 link_index;               // OS allocated index
  __u32 link_type;                // port type
  __u8 mac_addr[ETH_ALEN];        // mac address
  bool link;                      // lowerlayer state
  bool state;                     // administrative state
  __u32 mtu;                      // maximum transfer unit
  char master[IF_NAMESIZE];       // master of this port if any
  char real[IF_NAMESIZE];         // underlying real dev info if any
  __u32 tun_id;                   // tunnel info if any
  char tun_src[INET6_ADDRSTRLEN]; //  tunnel source
  char tun_dst[INET6_ADDRSTRLEN]; // tunnel dest
} api_port_mod_t;

#endif /* __FLB_API_DEFS_H__ */