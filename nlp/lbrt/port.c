#include <lbrt/port.h>

extern struct lbrt_net_meta mh;

lbrt_ports_h_t *lbrt_ports_h_alloc(void) {
  lbrt_ports_h_t *ph;
  ph = calloc(1, sizeof(*ph));
  if (!ph) {
    return NULL;
  }
  ph->port_mark = lbrt_counter_alloc(1, MaxInterfaces);
  ph->bond_mark = lbrt_counter_alloc(1, MaxBondInterfaces);
  ph->wg_mark = lbrt_counter_alloc(1, MaxWgInterfaces);
  ph->vti_mark = lbrt_counter_alloc(1, MaxVtiInterfaces);
  if (ph->port_mark == NULL || ph->bond_mark == NULL || ph->wg_mark == NULL ||
      ph->vti_mark == NULL) {
    lbrt_ports_h_free(ph);
    return NULL;
  }
  return ph;
}

void lbrt_ports_h_free(lbrt_ports_h_t *ph) {
  if (!ph)
    return;
  if (ph->port_mark) {
    free(ph->port_mark);
  }
  if (ph->bond_mark) {
    free(ph->bond_mark);
  }
  if (ph->wg_mark) {
    free(ph->wg_mark);
  }
  if (ph->vti_mark) {
    free(ph->vti_mark);
  }
  free(ph);
}

lbrt_port_t *lbrt_port_find_by_name(lbrt_ports_h_t *ph, const char *name) {
  lbrt_port_t *port = NULL;
  HASH_FIND(hh_by_name, ph->port_s_map, name, strlen(name), port);
  return port;
}

lbrt_port_t *lbrt_port_find_by_osid(lbrt_ports_h_t *ph, __u32 osid) {
  lbrt_port_t *port = NULL;
  HASH_FIND(hh_by_osid, ph->port_o_map, &osid, sizeof(osid), port);
  return port;
}

int lbrt_port_add(lbrt_ports_h_t *ph, char *name, __u32 osid, __u32 link_type,
                  char *zone, lbrt_port_hw_info_t *hwi,
                  lbrt_port_layer2_info_t *l2i) {
  if (!lbrt_zone_port_is_valid(mh.zn, name, zone))
    return PORT_ZONE_ERR;
  lbrt_zone_t *zn = lbrt_zone_find(mh.zn, zone);
  if (!zn)
    return PORT_ZONE_ERR;

  __u64 rid;
  if (link_type == PortBond) {
    rid = lbrt_counter_get_counter(ph->bond_mark);
  } else if (link_type == PortWg) {
    rid = lbrt_counter_get_counter(ph->wg_mark);
  } else if (link_type == PortVti) {
    rid = lbrt_counter_get_counter(ph->vti_mark);
  } else {
    rid = lbrt_counter_get_counter(ph->port_mark);
  }
  if (rid == COUNTER_OVERFLOW) {
    return PORT_COUNTER_ERR;
  }

  lbrt_port_t *rp = NULL;
  if (strlen(hwi->real) > 0) {
    rp = lbrt_port_find_by_name(ph, hwi->real);
    if (!rp) {
      printf("port add - %s no real-port(%s)\n", name, hwi->real);
      return PORT_NO_REAL_DEV_ERR;
    }
  } else if (link_type == PortVxlanBr) {
    return PORT_NO_REAL_DEV_ERR;
  }

  lbrt_port_t *p = calloc(1, sizeof(lbrt_port_t));
  memcpy(p->name, name, strlen(name));
  memcpy(p->zone, zone, strlen(zone));
  p->port_no = (__u32)rid;
  memcpy(&p->hinfo, hwi, sizeof(lbrt_port_hw_info_t));
  p->sinfo.port_active = true;
  p->sinfo.os_id = osid;
  p->sinfo.port_type = link_type;
  p->sinfo.port_real = rp;

  __u8 vmac[ETH_ALEN] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  switch (link_type) {
  case PortReal:
    break;
  case PortBond:
    break;
  case PortWg:
    break;
  case PortVti:
    break;
  case PortVxlanBr:
    break;
  case PortIPTun:
    break;
  default:
    break;
  }

  ph->port_i_map[rid] = p;
  HASH_ADD(hh_by_name, ph->port_s_map, name, strlen(name), p);
  HASH_ADD(hh_by_osid, ph->port_o_map, osid, sizeof(__u32), p);

  {
    lbrt_port_t *p, *tmp;
    HASH_ITER(hh_by_name, ph->port_s_map, p, tmp) {
      flb_log(LOG_LEVEL_DEBUG,
              YELLOW "hh_by_name port name=[%s] port no=[%d]" RESET, p->name,
              p->port_no);
    }
  }

  {
    lbrt_port_t *p, *tmp;
    HASH_ITER(hh_by_osid, ph->port_o_map, p, tmp) {
      flb_log(LOG_LEVEL_DEBUG,
              YELLOW "hh_by_osid port name=[%s] port no=[%d]" RESET, p->name,
              p->port_no);
    }
  }

  lbrt_zone_port_add(mh.zn, name, zone);

  flb_log(LOG_LEVEL_INFO, "port added - %s:%d\n", name, p->port_no);

  return 0;
}