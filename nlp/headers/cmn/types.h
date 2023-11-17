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

/* Length of zone name.  */
#define ZONE_NAMESIZE 16

/* Length of mirror name.  */
#define MIRR_NAMESIZE 50

/* Length of pol name.  */
#define POL_NAMESIZE 50

#define IF_ADDRSIZE 46
#define IF_CIDRSIZE 50
#define IF_MACADDRSIZE 18

#define IP4_ALEN 4
#define IP6_ALEN 16

typedef struct nl_ip {
  struct {
    __u8 v4 : 1;
    __u8 v6 : 1;
  } f;
  union {
    union {
      __u8 bytes[IP6_ALEN];
      __u32 s_addr[IP4_ALEN];
    } v6;
    union {
      __u8 _pad[IP6_ALEN - IP4_ALEN];
      union {
        __u8 bytes[IP4_ALEN];
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

static __u8 zero_mac[ETH_ALEN] = {0x00};

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
                                struct nl_ip_net *ip_net, struct nl_ip *ip) {
  memset(ip_net, 0, sizeof(*ip_net));
  if (ip) {
    memset(ip, 0, sizeof(*ip));
  }

  char *mask_str = strchr(ip_net_str, '/');
  if (!mask_str) {
    return false;
  }

  __u8 ip_str[INET6_ADDRSTRLEN] = {0x00};
  memcpy(ip_str, ip_net_str, mask_str - ip_net_str);
  if (strchr((char *)ip_str, '.')) {
    ip_net->ip.f.v4 = 1;
    if (inet_pton(AF_INET, (char *)ip_str, (char *)ip_net->ip.v4.bytes) <= 0) {
      return false;
    }
  } else if (strchr((char *)ip_str, ':')) {
    ip_net->ip.f.v6 = 1;
    if (inet_pton(AF_INET6, (char *)ip_str, (char *)ip_net->ip.v6.bytes) <= 0) {
      return false;
    }
  } else {
    return false;
  }

  if (ip) {
    memcpy(ip, &ip_net->ip, sizeof(ip_t));
  }

  mask_str++;
  ip_net->mask = (__u8)atoi(mask_str);

  int len = ip_net->ip.f.v4 ? IP4_ALEN : IP6_ALEN;
  __u8 mask_bytes[len];
  memset(mask_bytes, 0, len);
  __u32 n = (__u32)ip_net->mask;
  for (int i = 0; i < len; i++) {
    if (n >= 8) {
      mask_bytes[i] = 0xff;
      n -= 8;
    } else {
      mask_bytes[i] = ~(__u8)(0xff >> n);
      n = 0;
    }
  }

  if (ip_net->ip.f.v4) {
    for (int i = 0; i < IP4_ALEN; i++) {
      ip_net->ip.v4.bytes[i] = ip_net->ip.v4.bytes[i] & mask_bytes[i];
    }
  } else if (ip_net->ip.f.v6) {
    for (int i = 0; i < IP6_ALEN; i++) {
      ip_net->ip.v6.bytes[i] = ip_net->ip.v6.bytes[i] & mask_bytes[i];
    }
  }

  return true;
}

static inline bool parse_label_net(const char *ip_net_str,
                                   struct nl_label_net *ip_net,
                                   struct nl_ip *ip) {
  memset(ip_net, 0, sizeof(*ip_net));
  char *label_str = strchr(ip_net_str, ' ');
  if (!label_str) {
    return parse_ip_net(ip_net_str, (struct nl_ip_net *)ip_net, ip);
  }
  __u8 ip_str[INET6_ADDRSTRLEN] = {0};
  memcpy(ip_str, ip_net_str, label_str - ip_net_str);
  parse_ip_net((const char *)ip_str, (struct nl_ip_net *)ip_net, ip);
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

static inline char *mac_ntop(__u8 *mac) {
  static char str[IF_MACADDRSIZE];
  memset(str, 0, IF_MACADDRSIZE);
  snprintf(str, IF_MACADDRSIZE, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  return str;
}

static inline void ip_ntoa(ip_t *ip, char *str) {
  memset(str, 0, IF_ADDRSIZE);
  if (ip->f.v4) {
    struct in_addr *in = (struct in_addr *)ip->v4.bytes;
    char *ip_str = inet_ntoa(*in);
    snprintf(str, IF_ADDRSIZE, "%s", ip_str);
  } else if (ip->f.v6) {
    struct in6_addr *in = (struct in6_addr *)ip->v6.bytes;
    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, in, ip_str, INET6_ADDRSTRLEN);
    snprintf(str, IF_ADDRSIZE, "%s", ip_str);
  }
}

static inline void ip_net_ntoa(ip_net_t *ipnet, char *str) {
  memset(str, 0, IF_CIDRSIZE);
  if (ipnet->ip.f.v4) {
    struct in_addr *in = (struct in_addr *)ipnet->ip.v4.bytes;
    char *ip_str = inet_ntoa(*in);
    snprintf(str, IF_CIDRSIZE, "%s/%d", ip_str, ipnet->mask);
  } else if (ipnet->ip.f.v6) {
    struct in6_addr *in = (struct in6_addr *)ipnet->ip.v6.bytes;
    char ip_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, in, ip_str, INET6_ADDRSTRLEN);
    snprintf(str, IF_CIDRSIZE, "%s/%d", ip_str, ipnet->mask);
  }
}

static inline bool ip_net_contains(ip_net_t *ipnet, ip_t *ip) {
  if (ipnet->ip.f.v4 != ip->f.v4 || ipnet->ip.f.v6 != ip->f.v6) {
    return false;
  }
  int len = ipnet->ip.f.v4 ? IP4_ALEN : IP6_ALEN;
  __u8 mask_bytes[len];
  memset(mask_bytes, 0, len);
  __u32 n = (__u32)ipnet->mask;
  for (int i = 0; i < len; i++) {
    if (n >= 8) {
      mask_bytes[i] = 0xff;
      n -= 8;
    } else {
      mask_bytes[i] = ~(__u8)(0xff >> n);
      n = 0;
    }
  }

  if (ipnet->ip.f.v4) {
    for (int i = 0; i < IP4_ALEN; i++) {
      if ((ipnet->ip.v4.bytes[i] & mask_bytes[i]) !=
          (ip->v4.bytes[i] & mask_bytes[i]))
        return false;
    }
  }
  if (ipnet->ip.f.v6) {
    for (int i = 0; i < IP6_ALEN; i++) {
      if ((ipnet->ip.v6.bytes[i] & mask_bytes[i]) !=
          (ip->v6.bytes[i] & mask_bytes[i]))
        return false;
    }
  }
  return true;
}

#endif /* __FLB_CMN_TYPES_H__ */
