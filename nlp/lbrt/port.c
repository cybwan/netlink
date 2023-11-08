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
      flb_log(LOG_LEVEL_ERR, "port add - %s no real-port(%s)\n", name,
              hwi->real);
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

  //__u8 vmac[ETH_ALEN] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
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
      flb_log(LOG_LEVEL_DEBUG, "hh_by_name port name=[%s] port no=[%d]",
              p->name, p->port_no);
    }
  }

  {
    lbrt_port_t *p, *tmp;
    HASH_ITER(hh_by_osid, ph->port_o_map, p, tmp) {
      flb_log(LOG_LEVEL_DEBUG, "hh_by_osid port name=[%s] port no=[%d]",
              p->name, p->port_no);
    }
  }

  lbrt_zone_port_add(mh.zn, name, zone);

  flb_log(LOG_LEVEL_INFO, "port added - %s:%d", name, p->port_no);

  return 0;
}

int lbrt_port_del(lbrt_ports_h_t *ph, char *name, __u32 link_type) {
  lbrt_port_t *p = lbrt_port_find_by_name(ph, name);
  if (!p) {
    flb_log(LOG_LEVEL_ERR, "port delete - %s no such port", name);
    return PORT_NOT_EXIST_ERR;
  }

  if ((p->sinfo.port_type & (PortReal | PortVlanSif)) ==
          (PortReal | PortVlanSif) &&
      link_type == PortVlanSif) {
    lbrt_port_datapath(p, DP_REMOVE);

    p->sinfo.port_type = p->sinfo.port_type & (~PortVlanSif);
    if (p->sinfo.port_real) {
      free(p->sinfo.port_real);
      p->sinfo.port_real = NULL;
    }
    memset(p->hinfo.master, 0, IF_NAMESIZE);
    p->l2.is_p_vid = true;
    p->l2.vid = p->port_no + RealPortIDB;
    lbrt_port_datapath(p, DP_CREATE);
    return 0;
  }

  if ((p->sinfo.port_type & (PortVxlanBr | PortVlanSif)) ==
          (PortVxlanBr | PortVlanSif) &&
      link_type == PortVxlanBr) {
    lbrt_port_datapath(p, DP_REMOVE);

    p->sinfo.port_type = p->sinfo.port_type & (~PortVlanSif);
    memset(p->hinfo.master, 0, IF_NAMESIZE);
    p->l2.is_p_vid = true;
    p->l2.vid = p->hinfo.tun_id;
    lbrt_port_datapath(p, DP_CREATE);
    return 0;
  }

  if ((p->sinfo.port_type & (PortBond | PortVlanSif)) ==
          (PortBond | PortVlanSif) &&
      link_type == PortVlanSif) {
    lbrt_port_datapath(p, DP_REMOVE);

    p->sinfo.port_type = p->sinfo.port_type & (~PortVlanSif);
    p->l2.is_p_vid = true;
    p->l2.vid = p->port_no + BondIDB;
    lbrt_port_datapath(p, DP_CREATE);
    return 0;
  }

  if ((p->sinfo.port_type & (PortReal | PortBondSif)) ==
          (PortReal | PortBondSif) &&
      link_type == PortBondSif) {
    lbrt_port_datapath(p, DP_REMOVE);

    p->sinfo.port_type = p->sinfo.port_type & (~PortBondSif);
    memset(p->hinfo.master, 0, IF_NAMESIZE);
    p->l2.is_p_vid = true;
    p->l2.vid = p->port_no + RealPortIDB;
    lbrt_port_datapath(p, DP_CREATE);
    return 0;
  }

  __u32 rid = p->port_no;
  if (!ph->port_i_map[rid]) {
    flb_log(LOG_LEVEL_ERR, "port delete - %s no such osid", name);
    return PORT_MAP_ERR;
  }

  lbrt_port_datapath(p, DP_REMOVE);

  lbrt_zone_t *zone = lbrt_zone_get_by_port(mh.zn, p->name);
  switch (p->sinfo.port_type) {
  case PortVxlanBr:
    if (p->sinfo.port_real) {
      if (p->sinfo.port_real->sinfo.port_ovl) {
        free(p->sinfo.port_real->sinfo.port_ovl);
        p->sinfo.port_real->sinfo.port_ovl = NULL;
      }
      free(p->sinfo.port_real);
      p->sinfo.port_real = NULL;
    }
    break;
  case PortReal:
  case PortBond:
  case PortWg:
  case PortVti:
    if (zone) {
      // lbrt_vlan_del(zone->vlans,p->l2.vid);
    }
    break;
  case PortIPTun:
    if (zone) {
      // zone.Sess.Mark.PutCounter(p.SInfo.SessMark)
    }
    break;
  }

  if (p->sinfo.port_real) {
    free(p->sinfo.port_real);
    p->sinfo.port_real = NULL;
  }
  p->sinfo.port_active = false;
  lbrt_zone_port_del(mh.zn, name);

  flb_log(LOG_LEVEL_DEBUG, "port deleted - %s:%d", name, p->port_no);

  HASH_DELETE(hh_by_name, ph->port_s_map, p);
  HASH_DELETE(hh_by_osid, ph->port_o_map, p);
  ph->port_i_map[rid] = NULL;

  if (zone) {
    // zone.Rt.RtDeleteByPort(p.Name)
    // zone.Nh.NeighDeleteByPort(p.Name)
    // zone.L3.IfaDeleteAll(p.Name)
  }

  free(p);

  return 0;
}

int lbrt_port_datapath(lbrt_port_t *port, enum lbrt_dp_work work) { return 0; }