/*
 * netlink/netlink-compat.h	Netlink Compatability
 */

#ifndef NETLINK_COMPAT_H_
#define NETLINK_COMPAT_H_

#ifndef __USE_GNU
/* User visible structure for SCM_CREDENTIALS message */
struct ucred
{
  pid_t pid;			/* PID of sending process.  */
  uid_t uid;			/* UID of sending process.  */
  gid_t gid;			/* GID of sending process.  */
};
#endif

#if !defined _LINUX_SOCKET_H && !defined _BITS_SOCKADDR_H
typedef unsigned short  sa_family_t;
#endif

#ifndef IFNAMSIZ 
/** Maximum length of a interface name */
#define IFNAMSIZ 16
#endif

/* patch 2.4.x if_arp */
#ifndef ARPHRD_INFINIBAND
#define ARPHRD_INFINIBAND 32
#endif

/* patch 2.4.x eth header file */
#ifndef ETH_P_MPLS_UC
#define ETH_P_MPLS_UC  0x8847 
#endif

#ifndef ETH_P_MPLS_MC
#define ETH_P_MPLS_MC   0x8848
#endif

#ifndef  ETH_P_EDP2
#define ETH_P_EDP2      0x88A2
#endif

#ifndef ETH_P_HDLC
#define ETH_P_HDLC      0x0019 
#endif

#ifndef AF_LLC
#define AF_LLC		26
#endif

#endif
