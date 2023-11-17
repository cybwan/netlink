#include <arp.h>

int flb_arp_grat_addr(const char *adv_ipv4, const char *ifname) {
  ip_t adv_addr;
  if (!parse_ip(adv_ipv4, &adv_addr) || !adv_addr.f.v4) {
    flb_log(LOG_LEVEL_ERR, "invalid adv ipv4 (%s)", adv_ipv4);
    return -1;
  }
  return flb_arp_grat(&adv_addr, ifname);
}

int flb_arp_grat(ip_t *adv_addr, const char *ifname) {
  if (!adv_addr || !adv_addr->f.v4) {
    flb_log(LOG_LEVEL_ERR, "invalid adv ipv4");
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
  ll.sll_halen = ETH_ALEN;
  memset(ll.sll_addr, 0xff, 8);

  ret = bind(fd, (struct sockaddr *)&ll, sizeof(ll));
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "could not bind (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  flb_arp_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  msg.ar_hrd = htons(0x0001);
  msg.ar_pro = htons(0x0800);

  msg.ar_hln = ETH_ALEN;
  msg.ar_pln = IP4_ALEN;

  msg.ar_op = htons(ARPOP_REPLY);

  memcpy(msg.ar_sha, port.hwaddr, ETH_ALEN);
  memcpy(msg.ar_sip, adv_addr->v4.bytes, IP4_ALEN);

  memset(msg.ar_tha, 0xff, ETH_ALEN);
  memcpy(msg.ar_tip, adv_addr->v4.bytes, IP4_ALEN);

  ret = sendto(fd, &msg, sizeof(msg), 0, (struct sockaddr *)&ll, sizeof(ll));
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "error on sending packet: (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  close(fd);

  return 0;
}

int flb_arp_ping_addr(const char *dst_ipv4, const char *src_ipv4,
                      const char *ifname) {
  ip_t dst_addr;
  if (!parse_ip(dst_ipv4, &dst_addr) || !dst_addr.f.v4) {
    flb_log(LOG_LEVEL_ERR, "invalid dst ipv4 (%s)", dst_ipv4);
    return -1;
  }

  ip_t src_addr;
  if (!parse_ip(src_ipv4, &src_addr) || !src_addr.f.v4) {
    flb_log(LOG_LEVEL_ERR, "invalid src ipv4 (%s)", src_ipv4);
    return -1;
  }
  return flb_arp_ping(&dst_addr, &src_addr, ifname);
}

int flb_arp_ping(ip_t *dst_addr, ip_t *src_addr, const char *ifname) {
  if (!dst_addr || !dst_addr->f.v4) {
    flb_log(LOG_LEVEL_ERR, "invalid dst ipv4");
    return -1;
  }

  if (!src_addr || !src_addr->f.v4) {
    flb_log(LOG_LEVEL_ERR, "invalid src ipv4");
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
  ll.sll_halen = ETH_ALEN;
  memset(ll.sll_addr, 0xff, 8);

  ret = bind(fd, (struct sockaddr *)&ll, sizeof(ll));
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "could not bind (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  flb_arp_msg_t msg;
  memset(&msg, 0, sizeof(msg));
  msg.ar_hrd = htons(0x0001);
  msg.ar_pro = htons(0x0800);

  msg.ar_hln = ETH_ALEN;
  msg.ar_pln = IP4_ALEN;

  msg.ar_op = htons(ARPOP_REQUEST);

  memcpy(msg.ar_sha, port.hwaddr, ETH_ALEN);
  memcpy(msg.ar_sip, src_addr->v4.bytes, IP4_ALEN);

  memset(msg.ar_tha, 0x00, ETH_ALEN);
  memcpy(msg.ar_tip, dst_addr->v4.bytes, IP4_ALEN);

  ret = sendto(fd, &msg, sizeof(msg), 0, (struct sockaddr *)&ll, sizeof(ll));
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "error on sending packet: (%s)", strerror(ret));
    close(fd);
    return -1;
  }

  close(fd);

  return 0;
}