/*
 * netlink/data.h	Abstract Data
 */

#ifndef NETLINK_DATA_H_
#define NETLINK_DATA_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nl_data;

/* General */
extern struct nl_data *	nl_data_alloc(void *, size_t);
extern struct nl_data * nl_data_alloc_attr(struct nlattr *);
extern struct nl_data *	nl_data_clone(struct nl_data *);
extern int		nl_data_append(struct nl_data *, void *, size_t);
extern void		nl_data_free(struct nl_data *);

/* Access Functions */
extern void *		nl_data_get(struct nl_data *);
extern size_t		nl_data_get_size(struct nl_data *);

/* Misc */
extern int		nl_data_cmp(struct nl_data *, struct nl_data *);

#ifdef __cplusplus
}
#endif

#endif
