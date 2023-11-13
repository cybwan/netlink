#include <test.h>

extern struct lbrt_net_meta mh;

int test_l2_main(void) {
  int ret;

  lbrt_iter_intf_t tf;
  lbrt_iter_intf_init(&tf);

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

  lbrt_fdb_key_t fdb_key;
  memset(&fdb_key, 0, sizeof(fdb_key));
  memcpy(fdb_key.mac_addr, (__u8[6]){0x05, 0x04, 0x03, 0x3, 0x1, 0x0},
         ETH_ALEN);
  fdb_key.bridge_id = 100;

  lbrt_fdb_attr_t fdb_attr;
  memset(&fdb_attr, 0, sizeof(fdb_attr));
  memcpy(fdb_attr.oif, "hs0", sizeof("hs0"));
  fdb_attr.type = FdbVlan;
  parse_ip("0.0.0.0", &fdb_attr.dst);

  flb_log(LOG_LEVEL_INFO, "#### FDB ADD ####");
  ret = lbrt_fdb_add(mh.zr->l2, &fdb_key, &fdb_attr);
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add fdb hs0:vlan100");
  }

  flb_log(LOG_LEVEL_INFO, "#### FDB ADD ####");
  ret = lbrt_fdb_add(mh.zr->l2, &fdb_key, &fdb_attr);
  if (ret == 0) {
    flb_log(LOG_LEVEL_ERR, "added duplicate fdb vlan100");
  }

  flb_log(LOG_LEVEL_INFO, "#### FDB List ####");
  lbrt_fdbs_2_str(mh.zr->l2, tf);

  flb_log(LOG_LEVEL_INFO, "#### FDB DEL ####");
  ret = lbrt_fdb_del(mh.zr->l2, &fdb_key);
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add del hs0:vlan100");
  }

  flb_log(LOG_LEVEL_INFO, "#### FDB List ####");
  lbrt_fdbs_2_str(mh.zr->l2, tf);

  return 0;
}