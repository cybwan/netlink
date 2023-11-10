
#include <test.h>

extern struct lbrt_net_meta mh;

int test_l3_main(void) {
  int ret;

  __u8 ifmac[6] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
  lbrt_port_hw_info_t hwi;
  memset(&hwi, 0, sizeof(hwi));
  memcpy(hwi.mac_addr, ifmac, ETH_ALEN);
  hwi.link = true;
  hwi.state = true;
  hwi.mtu = 1500;

  lbrt_port_layer2_info_t l2i;
  memset(&l2i, 0, sizeof(l2i));
  l2i.is_p_vid = false;
  l2i.vid = 10;

  ret = lbrt_port_add(mh.zr->ports, "hs0", 12, PortReal, ROOT_ZONE, &hwi, &l2i);
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add port %s, ret=%d", "hs0", ret);
  }

  lbrt_port_t *p = lbrt_port_find_by_name(mh.zr->ports, "hs0");
  if (!p) {
    flb_log(LOG_LEVEL_ERR, "failed to add port %s", "hs0");
  }

  ret = lbrt_ifa_add(mh.zr->l3, "hs0", "11.11.11.1/24");
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add l3 ifa to hs0");
  }

  ret = lbrt_ifa_add(mh.zr->l3, "hs0", "11.11.11.2/24");
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add l3 ifa to hs0");
  }

  lbrt_iter_intf_t tf;
  lbrt_iter_intf_init(&tf);

  lbrt_ifas_2_str(mh.zr->l3, tf);

  flb_log(LOG_LEVEL_INFO, "TEST L3 DONE!");
  return 0;
}