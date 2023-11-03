#include <lbrt/zone.h>

#define MaximumZones 256

extern struct lbrt_net_meta *mh;

struct lbrt_zone_h *lbrt_zone_h_alloc(void) {
  struct lbrt_zone_h *zh;
  zh = calloc(1, sizeof(*zh));
  if (!zh) {
    return NULL;
  }
  zh->zone_map = NULL;
  zh->zone_brs = NULL;
  zh->zone_ports = NULL;
  zh->zone_mark = lbrt_counter_alloc(1, MaximumZones);
  if (zh->zone_mark == NULL) {
    free(zh);
    return NULL;
  }
  return zh;
}

void lbrt_zone_h_free(struct lbrt_zone_h *zh) {
  if (!zh)
    return;
  if (zh->zone_mark)
    lbrt_counter_free(zh->zone_mark);
  free(zh);
}