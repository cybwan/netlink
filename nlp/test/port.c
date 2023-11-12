
#include <test.h>

extern struct lbrt_net_meta mh;

int test_port_main(void) {
  int ret;

  __u8 *ifmac;
  lbrt_port_hw_info_t hwi;
  lbrt_port_layer2_info_t l2i;
  lbrt_port_t *p;

  lbrt_iter_intf_t tf;
  lbrt_iter_intf_init(&tf);

  { // hs0
    ifmac = (__u8[6]){0x1, 0x2, 0x3, 0x4, 0x5, 0x6};

    memset(&hwi, 0, sizeof(hwi));
    memcpy(hwi.mac_addr, ifmac, ETH_ALEN);
    hwi.link = true;
    hwi.state = true;
    hwi.mtu = 1500;

    memset(&l2i, 0, sizeof(l2i));
    l2i.is_p_vid = false;
    l2i.vid = 10;

    ret =
        lbrt_port_add(mh.zr->ports, "hs0", 12, PortReal, ROOT_ZONE, &hwi, &l2i);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "failed to add port %s, ret=%d", "hs0", ret);
    }

    p = lbrt_port_find_by_name(mh.zr->ports, "hs0");
    if (!p) {
      flb_log(LOG_LEVEL_ERR, "failed to add port %s", "hs0");
    }
  }

  { // bond1
    ifmac = (__u8[6]){0x1, 0x2, 0x3, 0x4, 0x5, 0x7};

    memset(&hwi, 0, sizeof(hwi));
    memcpy(hwi.mac_addr, ifmac, ETH_ALEN);
    hwi.link = true;
    hwi.state = true;
    hwi.mtu = 1500;

    memset(&l2i, 0, sizeof(l2i));
    l2i.is_p_vid = false;
    l2i.vid = 10;

    ret = lbrt_port_add(mh.zr->ports, "bond1", 15, PortReal, ROOT_ZONE, &hwi,
                        &l2i);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "failed to add port %s, ret=%d", "bond1", ret);
    }

    p = lbrt_port_find_by_name(mh.zr->ports, "bond1");
    if (!p) {
      flb_log(LOG_LEVEL_ERR, "failed to add port %s", "bond1");
    }
  }

  { // hs1
    memset(&hwi, 0, sizeof(hwi));
    memcpy(hwi.mac_addr, ifmac, ETH_ALEN);
    hwi.link = true;
    hwi.state = true;
    hwi.mtu = 1500;

    memset(&l2i, 0, sizeof(l2i));
    l2i.is_p_vid = false;
    l2i.vid = 10;

    ret =
        lbrt_port_add(mh.zr->ports, "hs1", 15, PortReal, ROOT_ZONE, &hwi, &l2i);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "failed to add port hs1");
    }

    memcpy(hwi.master, "bond1", strlen("bond1"));
    ret = lbrt_port_add(mh.zr->ports, "hs1", 15, PortBondSif, ROOT_ZONE, &hwi,
                        &l2i);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "failed to add port hs1 to bond1");
    }
  }

  { // hs2
    ifmac = (__u8[6]){0x1, 0x2, 0x3, 0x4, 0x5, 0x8};

    memset(&hwi, 0, sizeof(hwi));
    memcpy(hwi.mac_addr, ifmac, ETH_ALEN);
    hwi.link = true;
    hwi.state = true;
    hwi.mtu = 1500;

    memset(&l2i, 0, sizeof(l2i));
    l2i.is_p_vid = false;
    l2i.vid = 10;

    ret = lbrt_port_add(mh.zr->ports, "hs2", 100, PortReal, ROOT_ZONE, &hwi,
                        &l2i);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "failed to add port %s, ret=%d", "hs2", ret);
    }
  }

  { // hs4
    ifmac = (__u8[6]){0x1, 0x2, 0x3, 0x4, 0x5, 0x8};

    memset(&hwi, 0, sizeof(hwi));
    memcpy(hwi.mac_addr, ifmac, ETH_ALEN);
    hwi.link = true;
    hwi.state = true;
    hwi.mtu = 1500;
    memcpy(hwi.real, "hs4", strlen("hs4"));

    memset(&l2i, 0, sizeof(l2i));
    l2i.is_p_vid = false;
    l2i.vid = 10;

    ret = lbrt_port_add(mh.zr->ports, "hs4", 400, PortReal, ROOT_ZONE, &hwi,
                        &l2i);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "failed to add port %s, ret=%d", "hs4", ret);
    }
  }

  flb_log(LOG_LEVEL_INFO, "#### IF List ####");
  lbrt_ports_2_str(mh.zr->ports, tf);

  flb_log(LOG_LEVEL_INFO, "#### IF DEL ALL ####");
  lbrt_port_destruct_all(mh.zr->ports);

  flb_log(LOG_LEVEL_INFO, "#### IF List ####");
  lbrt_ports_2_str(mh.zr->ports, tf);

  flb_log(LOG_LEVEL_INFO, "TEST Port DONE!");
  return 0;
}