#ifndef __FLB_LBRT_MUTEX_H__
#define __FLB_LBRT_MUTEX_H__

#include <lbrt/types.h>

typedef struct lbrt_mutex {
  void (*lock)();
  void (*unlock)();
} lbrt_mutex_t;

#endif /* __FLB_LBRT_MUTEX_H__ */