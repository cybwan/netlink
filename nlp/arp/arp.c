#include <arp.h>

int flb_grat_arp_req(const char *adv_ipv4, const char *ifname) {
  ip_t adv_addr;
  if (!parse_ip(adv_ipv4, &adv_addr) && adv_addr.f.v4) {
    flb_log(LOG_LEVEL_ERR, "invalid adv ipv4 (%s)", adv_ipv4);
    return -1;
  }

  nl_port_mod_t port;
  memset(&port, 0, sizeof(port));
  int ret = nl_link_get_by_name(ifname, &port);
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "could not get device (%s)", ifname);
    return -1;
  }

  int fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP));
  if (!fd) {
    return -1;
  }

  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "could not set SO_RCVTIMEO (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname));
  if (ret != 0) {
    flb_log(LOG_LEVEL_ERR, "could not set SO_BINDTODEVICE (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  struct sockaddr_ll ll;
  memset(&ll, 0, sizeof(ll));
  ll.sll_family = AF_PACKET;
  ll.sll_protocol = htons(ETH_P_ARP);
  ll.sll_ifindex = port.index;
  ll.sll_pkttype = 0;
  ll.sll_hatype = 1;
  ll.sll_halen = 6;
  memset(ll.sll_addr, 0xff, 8);

  ret = bind(fd, (struct sockaddr *)&ll, sizeof(ll));
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "could not bind (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  grat_arp_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  msg.hardware_type = htons(0x0001);
  msg.protocol_type = htons(0x0800);

  msg.hardware_address_length = 6;
  msg.protocol_address_length = 4;

  msg.arp_options = htons(0x0002);

  memcpy(msg.src_hardware_address, port.hwaddr, ETH_ALEN);
  memcpy(msg.src_protocol_address, adv_addr.v4.bytes, 4);

  memset(msg.tgt_hardware_address, 0xff, ETH_ALEN);
  memcpy(msg.tgt_protocol_address, adv_addr.v4.bytes, 4);

  ret = sendto(fd, &msg, sizeof(msg), 0, (struct sockaddr *)&ll, sizeof(ll));
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "error on sending packet: (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  close(fd);

  flb_log(LOG_LEVEL_DEBUG, "test");

  return 0;
}