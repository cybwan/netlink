#include <lbrt/zone.h>

#define MaximumZones 256

extern struct lbrt_net_meta mh;

lbrt_zone_h_t *lbrt_zone_h_new(void) {
  lbrt_zone_h_t *zh;
  zh = calloc(1, sizeof(*zh));
  if (!zh) {
    return NULL;
  }
  zh->zone_map = NULL;
  zh->zone_brs = NULL;
  zh->zone_ports = NULL;
  zh->zone_mark = lbrt_counter_new(1, MaximumZones);
  if (zh->zone_mark == NULL) {
    lbrt_zone_h_free(zh);
    return NULL;
  }
  return zh;
}

void lbrt_zone_h_free(lbrt_zone_h_t *zh) {
  if (!zh)
    return;
  if (zh->zone_mark)
    lbrt_counter_free(zh->zone_mark);
  free(zh);
}

int lbrt_zone_add(lbrt_zone_h_t *zh, const char *name) {
  lbrt_zone_t *zn = NULL;
  HASH_FIND(hh, zh->zone_map, name, strlen(name), zn);
  if (zn)
    return ZONE_EXISTS_ERR;

  zn = calloc(1, sizeof(*zn));
  if (!zn)
    return ZONE_ALLOC_ERR;

  __u64 zn_num = lbrt_counter_get_counter(zh->zone_mark);
  if (zn_num == COUNTER_OVERFLOW) {
    free(zn);
    return ZONE_NUMBER_ERR;
  }

  zn->zone_num = zn_num;
  memcpy(zn->name, name, strlen(name));
  zn->ports = lbrt_ports_h_new();
  zn->vlans = lbrt_vlans_h_new(zn);
  zn->l2 = lbrt_l2_h_new(zn);
  zn->nh = lbrt_neigh_h_new(zn);
  zn->rt = lbrt_rt_h_new(zn);
  zn->l3 = lbrt_l3_h_new(zn);
  // zn.Rules = RulesInit(zone)
  // zn.Sess = SessInit(zone)
  zn->pols = lbrt_pol_h_new(zn);
  zn->mirrs = lbrt_mirr_h_new(zn);

  HASH_ADD(hh, zh->zone_map, name, strlen(name), zn);

  return 0;
}

lbrt_zone_t *lbrt_zone_find(lbrt_zone_h_t *zh, const char *name) {
  lbrt_zone_t *zn = NULL;
  HASH_FIND(hh, zh->zone_map, name, strlen(name), zn);
  return zn;
}

int lbrt_zone_delete(lbrt_zone_h_t *zh, const char *name) {
  lbrt_zone_t *zn = NULL;
  HASH_FIND(hh, zh->zone_map, name, strlen(name), zn);
  if (!zn)
    return ZONE_NOT_EXIST_ERR;
  HASH_DEL(zh->zone_map, zn);
  return 0;
}

int lbrt_zone_br_add(lbrt_zone_h_t *zh, const char *br_name,
                     const char *zone_name) {
  lbrt_zone_br_t *zn_br = NULL;
  HASH_FIND(hh, zh->zone_brs, br_name, strlen(br_name), zn_br);
  if (zn_br)
    return ZONE_EXISTS_ERR;

  lbrt_zone_t *zn = lbrt_zone_find(zh, zone_name);
  if (!zn)
    return ZONE_NOT_EXIST_ERR;

  zn_br = calloc(1, sizeof(*zn_br));
  if (!zn_br)
    return ZONE_ALLOC_ERR;
  memcpy(zn_br->br_name, br_name, strlen(br_name));
  zn_br->zone = zn;
  HASH_ADD(hh, zh->zone_brs, br_name, strlen(br_name), zn_br);

  return 0;
}

int lbrt_zone_br_del(lbrt_zone_h_t *zh, const char *br_name) {
  lbrt_zone_br_t *zn_br = NULL;
  HASH_FIND(hh, zh->zone_brs, br_name, strlen(br_name), zn_br);
  if (!zn_br)
    return ZONE_NOT_EXIST_ERR;
  HASH_DEL(zh->zone_brs, zn_br);
  return 0;
}

int lbrt_zone_port_add(lbrt_zone_h_t *zh, const char *port_name,
                       const char *zone_name) {
  lbrt_zone_port_t *zn_port = NULL;
  HASH_FIND(hh, zh->zone_ports, port_name, strlen(port_name), zn_port);
  if (zn_port) {
    if (strcmp(zn_port->zone->name, zone_name) == 0)
      return 0;
    return ZONE_EXISTS_ERR;
  }

  lbrt_zone_t *zn = lbrt_zone_find(zh, zone_name);
  if (!zn)
    return ZONE_NOT_EXIST_ERR;

  zn_port = calloc(1, sizeof(*zn_port));
  if (!zn_port)
    return ZONE_ALLOC_ERR;
  memcpy(zn_port->port_name, port_name, strlen(port_name));
  zn_port->zone = zn;
  HASH_ADD(hh, zh->zone_ports, port_name, strlen(port_name), zn_port);

  return 0;
}

int lbrt_zone_port_del(lbrt_zone_h_t *zh, const char *port_name) {
  lbrt_zone_br_t *zn_port = NULL;
  HASH_FIND(hh, zh->zone_ports, port_name, strlen(port_name), zn_port);
  if (!zn_port)
    return ZONE_NOT_EXIST_ERR;
  HASH_DEL(zh->zone_ports, zn_port);
  return 0;
}

bool lbrt_zone_port_is_valid(lbrt_zone_h_t *zh, const char *port_name,
                             const char *zone_name) {
  lbrt_zone_port_t *zn_port = NULL;
  HASH_FIND(hh, zh->zone_ports, port_name, strlen(port_name), zn_port);
  if (!zn_port)
    return true;

  if (strcmp(zn_port->zone->name, zone_name) == 0)
    return true;
  return false;
}

lbrt_zone_t *lbrt_zone_get_by_port(lbrt_zone_h_t *zh, const char *port_name) {
  lbrt_zone_br_t *zn_port = NULL;
  HASH_FIND(hh, zh->zone_ports, port_name, strlen(port_name), zn_port);
  if (!zn_port)
    return NULL;
  return zn_port->zone;
}