
#include <test.h>

extern struct lbrt_net_meta mh;

int test_l3_main(void) {
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

  ret = lbrt_ifa_add(mh.zr->l3, "hs0", "11.11.11.1/24");
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add l3 ifa to hs0");
  }

  ret = lbrt_ifa_add(mh.zr->l3, "hs0", "11.11.11.2/24");
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add l3 ifa to hs0");
  }

  ret = lbrt_ifa_add(mh.zr->l3, "hs0", "11.11.12.1/24");
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add l3 ifa to hs0");
  }

  ret = lbrt_ifa_add(mh.zr->l3, "hs0", "11.11.12.2/24");
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add l3 ifa to hs0");
  }

  flb_log(LOG_LEVEL_INFO, "#### IFA List ALL ####");
  lbrt_ifas_2_str(mh.zr->l3, tf);

  ret = lbrt_ifa_del(mh.zr->l3, "hs0", "11.11.11.2/24");
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "failed to add l3 ifa to hs0");
  }

  flb_log(LOG_LEVEL_INFO, "#### IFA List After DEL 11.11.11.2/24 ####");
  lbrt_ifas_2_str(mh.zr->l3, tf);

  bool any = false;
  char *target_addr = "11.11.12.100";
  ip_t target_ip;
  char via_dev[IF_NAMESIZE];
  ip_t via_ip;
  char via_addr[IF_ADDRSIZE];

  parse_ip(target_addr, &target_ip);
  ret = lbrt_ifa_select(mh.zr->l3, "hs0", &target_ip, any, &via_ip, via_dev);
  if (ret == 0) {
    ip_ntoa(&via_ip, via_addr);
    flb_log(LOG_LEVEL_DEBUG,
            "select ifa any=[%d] success to [%s] via [%s] dev [%s]", any,
            target_addr, via_addr, via_dev);
  } else {
    flb_log(LOG_LEVEL_ERR, "select ifa any=[%d] failure to [%s]", any,
            target_addr);
  }

  any = false;
  target_addr = "11.11.13.100";
  parse_ip(target_addr, &target_ip);
  ret = lbrt_ifa_select(mh.zr->l3, "hs0", &target_ip, any, &via_ip, via_dev);
  if (ret == 0) {
    ip_ntoa(&via_ip, via_addr);
    flb_log(LOG_LEVEL_DEBUG,
            "select ifa any=[%d] success to [%s] via [%s] dev [%s]", any,
            target_addr, via_addr, via_dev);
  } else {
    flb_log(LOG_LEVEL_DEBUG, "select ifa any=[%d] failure to [%s]", any,
            target_addr);
  }

  any = true;
  target_addr = "11.11.13.100";
  parse_ip(target_addr, &target_ip);
  ret = lbrt_ifa_select(mh.zr->l3, "hs0", &target_ip, any, &via_ip, via_dev);
  if (ret == 0) {
    ip_ntoa(&via_ip, via_addr);
    flb_log(LOG_LEVEL_DEBUG,
            "select ifa any=[%d] success to [%s] via [%s] dev [%s]", any,
            target_addr, via_addr, via_dev);
  } else {
    flb_log(LOG_LEVEL_ERR, "select ifa any=[%d] failure to [%s]", any,
            target_addr);
  }

  any = false;
  target_addr = "11.11.12.88";
  parse_ip(target_addr, &target_ip);
  ret = lbrt_ifa_select_any(mh.zr->l3, &target_ip, any, &via_ip, via_dev);
  if (ret == 0) {
    ip_ntoa(&via_ip, via_addr);
    flb_log(LOG_LEVEL_DEBUG,
            "select_any ifa any=[%d] success to [%s] via [%s] dev [%s]", any,
            target_addr, via_addr, via_dev);
  } else {
    flb_log(LOG_LEVEL_ERR, "select_any ifa any=[%d] failure to [%s]", any,
            target_addr);
  }

  any = false;
  target_addr = "11.11.14.88";
  parse_ip(target_addr, &target_ip);
  ret = lbrt_ifa_select_any(mh.zr->l3, &target_ip, any, &via_ip, via_dev);
  if (ret == 0) {
    ip_ntoa(&via_ip, via_addr);
    flb_log(LOG_LEVEL_DEBUG,
            "select_any ifa any=[%d] success to [%s] via [%s] dev [%s]", any,
            target_addr, via_addr, via_dev);
  } else {
    flb_log(LOG_LEVEL_DEBUG, "select_any ifa any=[%d] failure to [%s]", any,
            target_addr);
  }

  any = true;
  target_addr = "11.11.14.88";
  parse_ip(target_addr, &target_ip);
  ret = lbrt_ifa_select_any(mh.zr->l3, &target_ip, any, &via_ip, via_dev);
  if (ret == 0) {
    ip_ntoa(&via_ip, via_addr);
    flb_log(LOG_LEVEL_DEBUG,
            "select_any ifa any=[%d] success to [%s] via [%s] dev [%s]", any,
            target_addr, via_addr, via_dev);
  } else {
    flb_log(LOG_LEVEL_ERR, "select_any ifa any=[%d] failure to [%s]", any,
            target_addr);
  }

  char out_str[56];
  lbrt_ifa_obj_mk_str(mh.zr->l3, "hs0", true, out_str);
  flb_log(LOG_LEVEL_DEBUG, "mk_string ifa success to [%s]", out_str);

  lbrt_ifa_del_all(mh.zr->l3, "hs0");

  flb_log(LOG_LEVEL_INFO, "#### IFA List After DEL ALL ####");
  lbrt_ifas_2_str(mh.zr->l3, tf);

  flb_log(LOG_LEVEL_INFO, "TEST L3 DONE!");
  return 0;
}