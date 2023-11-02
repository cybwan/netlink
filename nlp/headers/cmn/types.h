#ifndef __FLB_CMN_TYPES_H__
#define __FLB_CMN_TYPES_H__

#include <ctype.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if.h>

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

typedef struct nl_ip {
  struct {
    __u8 v4 : 1;
    __u8 v6 : 1;
  } f;
  union {
    union {
      __u8 bytes[16];
      __u32 s_addr[4];
    } v6;
    union {
      __u8 _pad[12];
      union {
        __u8 bytes[4];
        __u32 s_addr;
      };
    } v4;
  };
} ip_t;

typedef struct nl_ip_net {
  struct nl_ip ip;
  __u8 mask;
} ip_net_t;

typedef struct nl_label_net {
  struct nl_ip ip;
  __u8 mask;
  __u8 label[IF_NAMESIZE];
} label_net_t;

static __u8 zero_mac[ETH_ALEN] = {0};

static inline bool is_zero_mac(__u8 mac[ETH_ALEN]) {
  if (memcmp(mac, zero_mac, ETH_ALEN) == 0) {
    return true;
  }
  return false;
}

static inline bool parse_ip(const char *ip_str, struct nl_ip *ip) {
  memset(ip, 0, sizeof(*ip));
  if (strchr(ip_str, '.')) {
    ip->f.v4 = 1;
    if (inet_pton(AF_INET, ip_str, (char *)ip->v4.bytes) <= 0) {
      return false;
    }
  } else if (strchr(ip_str, ':')) {
    ip->f.v6 = 1;
    if (inet_pton(AF_INET6, ip_str, (char *)ip->v6.bytes) <= 0) {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

static inline bool parse_ip_net(const char *ip_net_str,
                                struct nl_ip_net *ip_net) {
  memset(ip_net, 0, sizeof(*ip_net));
  char *mask_str = strchr(ip_net_str, '/');
  if (!mask_str) {
    return false;
  }
  __u8 ip[INET6_ADDRSTRLEN] = {0};
  memcpy(ip, ip_net_str, mask_str - ip_net_str);
  if (strchr((char *)ip, '.')) {
    ip_net->ip.f.v4 = 1;
    if (inet_pton(AF_INET, (char *)ip, (char *)ip_net->ip.v4.bytes) <= 0) {
      return false;
    }
  } else if (strchr((char *)ip, ':')) {
    ip_net->ip.f.v6 = 1;
    if (inet_pton(AF_INET6, (char *)ip, (char *)ip_net->ip.v6.bytes) <= 0) {
      return false;
    }
  } else {
    return false;
  }
  mask_str++;
  ip_net->mask = (__u8)atoi(mask_str);
  return true;
}

static inline bool parse_label_net(const char *ip_net_str,
                                   struct nl_label_net *ip_net) {
  memset(ip_net, 0, sizeof(*ip_net));
  char *label_str = strchr(ip_net_str, ' ');
  if (!label_str) {
    return parse_ip_net(ip_net_str, (struct nl_ip_net *)ip_net);
  }
  __u8 ip[INET6_ADDRSTRLEN] = {0};
  memcpy(ip, ip_net_str, label_str - ip_net_str);
  parse_ip_net((const char *)ip, (struct nl_ip_net *)ip_net);
  label_str++;
  memcpy(ip_net->label, label_str, strlen(label_str));
  return true;
}

static inline int hex_to_bin(__u8 ch) {
  __u8 cu = ch & 0xdf;
  return -1 +
         ((ch - '0' + 1) & (unsigned)((ch - '9' - 1) & ('0' - 1 - ch)) >> 8) +
         ((cu - 'A' + 11) & (unsigned)((cu - 'F' - 1) & ('A' - 1 - cu)) >> 8);
}

static inline bool mac_pton(const char *s, __u8 *mac) {
  int i;

  /* XX:XX:XX:XX:XX:XX */
  if (strlen(s) < 3 * ETH_ALEN - 1)
    return false;

  /* Don't dirty result unless string is valid MAC. */
  for (i = 0; i < ETH_ALEN; i++) {
    if (!isxdigit(s[i * 3]) || !isxdigit(s[i * 3 + 1]))
      return false;
    if (i != ETH_ALEN - 1 && s[i * 3 + 2] != ':')
      return false;
  }
  for (i = 0; i < ETH_ALEN; i++) {
    mac[i] = (hex_to_bin(s[i * 3]) << 4) | hex_to_bin(s[i * 3 + 1]);
  }
  return true;
}

#endif /* __FLB_CMN_TYPES_H__ */
