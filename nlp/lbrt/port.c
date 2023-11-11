#include <lbrt/port.h>

extern struct lbrt_net_meta mh;

lbrt_ports_h_t *lbrt_ports_h_new(void) {
  lbrt_ports_h_t *ph;
  ph = calloc(1, sizeof(*ph));
  if (!ph) {
    return NULL;
  }
  ph->port_mark = lbrt_counter_new(1, MaxInterfaces);
  ph->bond_mark = lbrt_counter_new(1, MaxBondInterfaces);
  ph->wg_mark = lbrt_counter_new(1, MaxWgInterfaces);
  ph->vti_mark = lbrt_counter_new(1, MaxVtiInterfaces);
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

static UT_icd __slaves_icd = {sizeof(lbrt_port_t *), NULL, NULL, NULL};

UT_array *lbrt_port_get_slaves(lbrt_ports_h_t *ph, const char *master) {
  UT_array *slaves;
  utarray_new(slaves, &__slaves_icd);

  lbrt_port_t *p, *tmp;
  HASH_ITER(hh_by_name, ph->port_s_map, p, tmp) {
    if (strcmp(p->hinfo.master, master) == 0) {
      utarray_push_back(slaves, p);
    }
  }

  return slaves;
}

bool lbrt_port_has_tun_slaves(lbrt_ports_h_t *ph, const char *master,
                              __u32 ptype) {
  bool ret = false;

  UT_array *slaves;
  utarray_new(slaves, &__slaves_icd);

  lbrt_port_t *p, *tmp;
  HASH_ITER(hh_by_name, ph->port_s_map, p, tmp) {
    if (strcmp(p->hinfo.master, master) == 0 &&
        (p->sinfo.port_type & ptype) == ptype) {
      utarray_push_back(slaves, p);
    }
  }

  if (utarray_len(slaves) > 0) {
    ret = true;
  }

  utarray_free(slaves);

  return ret;
}

int lbrt_port_add(lbrt_ports_h_t *ph, char *name, __u32 osid, __u32 link_type,
                  char *zone, lbrt_port_hw_info_t *hwi,
                  lbrt_port_layer2_info_t *l2i) {
  if (!lbrt_zone_port_is_valid(mh.zn, name, zone)) {
    flb_log(LOG_LEVEL_ERR, "port add - %s no such zone", name);
    return PORT_ZONE_ERR;
  }

  lbrt_zone_t *zn = lbrt_zone_find(mh.zn, zone);
  if (!zn) {
    flb_log(LOG_LEVEL_ERR, "port add - %s no such zone", name);
    return PORT_ZONE_ERR;
  }

  lbrt_port_t *p = lbrt_port_find_by_name(ph, name);
  if (p) {
    p->hinfo.link = hwi->link;
    p->hinfo.state = hwi->state;
    p->hinfo.mtu = hwi->mtu;

    if (!lbrt_port_is_l3_tun_port(p) &&
        memcmp(hwi->mac_addr, p->hinfo.mac_addr, ETH_ALEN) != 0) {
      memcpy(p->hinfo.mac_addr, hwi->mac_addr, ETH_ALEN);
      lbrt_port_datapath(p, DP_CREATE);
    }

    if (p->sinfo.port_type == PortReal) {
      if (link_type == PortVlanSif) {
        p->sinfo.port_type |= link_type;
        memcpy(p->hinfo.master, hwi->master, strlen(hwi->master));
        if (memcmp(&p->l2, l2i, sizeof(lbrt_port_layer2_info_t)) != 0) {
          lbrt_port_t *rp = NULL;
          bool lds = p->sinfo.bpf_loaded;
          if (strlen(hwi->real) > 0) {
            rp = lbrt_port_find_by_name(ph, hwi->real);
            if (!rp) {
              flb_log(LOG_LEVEL_ERR, "port add - %s no real-port(%s) sif", name,
                      hwi->real);
              return PORT_NO_REAL_DEV_ERR;
            }
          } else {
            p->sinfo.bpf_loaded = false;
          }

          lbrt_port_datapath(p, DP_REMOVE);

          memcpy(&p->l2, l2i, sizeof(lbrt_port_layer2_info_t));
          p->sinfo.port_real = rp;
          p->sinfo.bpf_loaded = lds;

          lbrt_port_datapath(p, DP_CREATE);

          flb_log(LOG_LEVEL_DEBUG, "port add - %s vinfo updated", name);

          return 0;
        }
      }

      if (link_type == PortBondSif) {
        lbrt_port_t *master = lbrt_port_find_by_name(ph, hwi->master);
        if (!master) {
          flb_log(LOG_LEVEL_ERR, "port add - %s no master(%s)", name,
                  hwi->master);
          return PORT_NO_MASTER_ERR;
        }

        lbrt_port_datapath(p, DP_REMOVE);

        p->sinfo.port_type |= link_type;
        memcpy(p->hinfo.master, hwi->master, strlen(hwi->master));
        p->l2.is_p_vid = true;
        p->l2.vid = master->port_no + BondIDB;

        return 0;
      }
    }

    if (p->sinfo.port_type == PortBond) {
      if (link_type == PortVlanSif && l2i->is_p_vid) {
        if (memcmp(&p->l2, l2i, sizeof(lbrt_port_layer2_info_t)) != 0) {
          lbrt_port_datapath(p, DP_REMOVE);

          p->sinfo.port_type |= link_type;
          memcpy(&p->l2, l2i, sizeof(lbrt_port_layer2_info_t));

          lbrt_port_datapath(p, DP_CREATE);

          return 0;
        }
      }
    }

    if (p->sinfo.port_type == PortVxlanBr) {
      if (link_type == PortVlanSif && l2i->is_p_vid) {
        memcpy(p->hinfo.master, hwi->master, strlen(hwi->master));
        p->sinfo.port_type |= link_type;

        lbrt_port_datapath(p, DP_REMOVE);

        memcpy(&p->l2, l2i, sizeof(lbrt_port_layer2_info_t));

        lbrt_port_datapath(p, DP_CREATE);

        flb_log(LOG_LEVEL_DEBUG, "port add - %s vxinfo updated", name);

        return 0;
      }
    }

    if ((p->sinfo.port_type & (PortReal | PortBondSif)) ==
        (PortReal | PortBondSif)) {
      if (link_type == PortReal) {
        p->l2.is_p_vid = true;
        p->l2.vid = p->port_no + RealPortIDB;
        p->sinfo.port_type &= ~PortBondSif;
        memset(p->hinfo.master, 0, IF_NAMESIZE);

        lbrt_port_datapath(p, DP_CREATE);

        return 0;
      }
    }

    flb_log(LOG_LEVEL_ERR, "port add - %s exists", name);
    return PORT_EXISTS_ERR;
  }

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
    flb_log(LOG_LEVEL_ERR, "port add - %s mark error", name);
    return PORT_COUNTER_ERR;
  }

  lbrt_port_t *rp = NULL;
  if (strlen(hwi->real) > 0) {
    rp = lbrt_port_find_by_name(ph, hwi->real);
    if (!rp) {
      flb_log(LOG_LEVEL_ERR, "port add - %s no real-port(%s)", name, hwi->real);
      return PORT_NO_REAL_DEV_ERR;
    }
  } else if (link_type == PortVxlanBr) {
    flb_log(LOG_LEVEL_ERR, "port add - %s real-port needed", name);
    return PORT_NO_REAL_DEV_ERR;
  }

  p = calloc(1, sizeof(lbrt_port_t));
  memcpy(p->name, name, strlen(name));
  memcpy(p->zone, zone, strlen(zone));
  p->port_no = (__u32)rid;
  memcpy(&p->hinfo, hwi, sizeof(lbrt_port_hw_info_t));
  p->sinfo.port_active = true;
  p->sinfo.osid = osid;
  p->sinfo.port_type = link_type;
  p->sinfo.port_real = rp;

  __u8 vmac[ETH_ALEN] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  switch (link_type) {
  case PortReal:
    p->l2.is_p_vid = true;
    p->l2.vid = (__u32)rid + RealPortIDB;

    { /* We create an vlan BD to keep things in sync */
      char vstr[IF_NAMESIZE];
      memset(vstr, 0, IF_NAMESIZE);
      snprintf(vstr, IF_NAMESIZE, "vlan%d", p->l2.vid);
      lbrt_port_hw_info_t vlan_hwi;
      memset(&vlan_hwi, 0, sizeof(vlan_hwi));
      memcpy(vlan_hwi.mac_addr, vmac, ETH_ALEN);
      vlan_hwi.link = true;
      vlan_hwi.state = true;
      vlan_hwi.mtu = 9000;
      lbrt_vlan_add(zn->vlans, p->l2.vid, vstr, zone, -1, &vlan_hwi);
    }

    break;
  case PortBond:
    p->l2.is_p_vid = true;
    p->l2.vid = (__u32)rid + BondIDB;

    { /* We create an vlan BD to keep things in sync */
      char vstr[IF_NAMESIZE];
      memset(vstr, 0, IF_NAMESIZE);
      snprintf(vstr, IF_NAMESIZE, "vlan%d", p->l2.vid);
      lbrt_port_hw_info_t vlan_hwi;
      memset(&vlan_hwi, 0, sizeof(vlan_hwi));
      memcpy(vlan_hwi.mac_addr, vmac, ETH_ALEN);
      vlan_hwi.link = true;
      vlan_hwi.state = true;
      vlan_hwi.mtu = 9000;
      lbrt_vlan_add(zn->vlans, p->l2.vid, vstr, zone, -1, &vlan_hwi);
    }

    break;
  case PortWg:
    p->l2.is_p_vid = true;
    p->l2.vid = (__u32)rid + WgIDB;

    { /* We create an vlan BD to keep things in sync */
      char vstr[IF_NAMESIZE];
      memset(vstr, 0, IF_NAMESIZE);
      snprintf(vstr, IF_NAMESIZE, "vlan%d", p->l2.vid);
      lbrt_port_hw_info_t vlan_hwi;
      memset(&vlan_hwi, 0, sizeof(vlan_hwi));
      memcpy(vlan_hwi.mac_addr, vmac, ETH_ALEN);
      vlan_hwi.link = true;
      vlan_hwi.state = true;
      vlan_hwi.mtu = 9000;
      lbrt_vlan_add(zn->vlans, p->l2.vid, vstr, zone, -1, &vlan_hwi);
    }

    break;
  case PortVti:
    p->l2.is_p_vid = true;
    p->l2.vid = (__u32)rid + VtIDB;

    { /* We create an vlan BD to keep things in sync */
      char vstr[IF_NAMESIZE];
      memset(vstr, 0, IF_NAMESIZE);
      snprintf(vstr, IF_NAMESIZE, "vlan%d", p->l2.vid);
      lbrt_port_hw_info_t vlan_hwi;
      memset(&vlan_hwi, 0, sizeof(vlan_hwi));
      memcpy(vlan_hwi.mac_addr, vmac, ETH_ALEN);
      vlan_hwi.link = true;
      vlan_hwi.state = true;
      vlan_hwi.mtu = 9000;
      lbrt_vlan_add(zn->vlans, p->l2.vid, vstr, zone, -1, &vlan_hwi);
    }

    break;
  case PortVxlanBr:
    if (p->sinfo.port_real) {
      p->sinfo.port_real->sinfo.port_ovl = p;
      p->sinfo.port_real->sinfo.port_type |= PortVxlanSif;
      memcpy(p->sinfo.port_real->hinfo.master, p->name, strlen(p->name));
    }
    p->l2.is_p_vid = true;
    p->l2.vid = p->hinfo.tun_id;

    break;
  case PortIPTun:
    memcpy(&p->l2, l2i, sizeof(lbrt_port_layer2_info_t));

    {
      __u64 sess_mark = lbrt_counter_get_counter(zn->sess->mark);
      if (sess_mark == COUNTER_OVERFLOW) {
        flb_log(LOG_LEVEL_ERR, "port add - %s sess-alloc fail", name);
        sess_mark = 0;
      }
      p->sinfo.sess_mark = sess_mark;
    }

    break;
  default:
    flb_log(LOG_LEVEL_DEBUG, "port add - %s isPvid %s", name,
            p->l2.is_p_vid ? "true" : "false");
    memcpy(&p->l2, l2i, sizeof(lbrt_port_layer2_info_t));

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

  lbrt_port_datapath(p, DP_CREATE);

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
    flb_log(LOG_LEVEL_ERR, "port delete - %s no such num", name);
    return PORT_MAP_ERR;
  }

  if (!lbrt_port_find_by_osid(ph, p->sinfo.osid)) {
    flb_log(LOG_LEVEL_ERR, "port delete - %s no such osid", name);
    return PORT_MAP_ERR;
  }

  lbrt_port_datapath(p, DP_REMOVE);

  lbrt_zone_t *zone = lbrt_zone_get_by_port(mh.zn, p->name);
  switch (p->sinfo.port_type) {
  case PortVxlanBr:
    if (p->sinfo.port_real) {
      p->sinfo.port_real->sinfo.port_ovl = NULL;
    }
    break;
  case PortReal:
  case PortBond:
  case PortWg:
  case PortVti:
    if (zone) {
      lbrt_vlan_del(zone->vlans, p->l2.vid);
    }
    break;
  case PortIPTun:
    if (zone) {
      lbrt_counter_put_counter(zone->sess->mark, p->sinfo.sess_mark);
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
    lbrt_rt_del_by_port(zone->rt, p->name);
    lbrt_neigh_del_by_port(zone->nh, p->name);
    lbrt_ifa_del_all(zone->l3, p->name);
  }

  free(p);

  return 0;
}

bool lbrt_port_l2_addr_match(lbrt_ports_h_t *ph, char *name, lbrt_port_t *mp) {
  lbrt_port_t *p = lbrt_port_find_by_name(ph, name);
  if (p) {
    if (memcmp(p->hinfo.mac_addr, mp->hinfo.mac_addr, ETH_ALEN) == 0) {
      return true;
    }
  }
  return false;
}

void __lbrt_port_2_str(lbrt_port_t *e, lbrt_iter_intf_t it) {
  UT_string *pStr, *s;
  utstring_new(pStr);
  utstring_new(s);

  if (e->hinfo.state) {
    utstring_printf(pStr, "UP");
  } else {
    utstring_printf(pStr, "DOWN");
  }

  if (e->hinfo.link) {
    utstring_printf(pStr, ",RUNNING");
  }

  utstring_printf(s, "%-10s: <%s> mtu %d %s\n", e->name, utstring_body(pStr),
                  e->hinfo.mtu, e->zone);

  utstring_clear(pStr);

  if ((e->sinfo.port_type & PortReal) == PortReal) {
    utstring_printf(pStr, "phy,");
  }
  if ((e->sinfo.port_type & PortVlanSif) == PortVlanSif) {
    utstring_printf(pStr, "vlan-sif,");
  }
  if ((e->sinfo.port_type & PortVlanBr) == PortVlanBr) {
    utstring_printf(pStr, "vlan,");
  }
  if ((e->sinfo.port_type & PortBondSif) == PortBondSif) {
    utstring_printf(pStr, "bond-sif,");
  }
  if ((e->sinfo.port_type & PortBond) == PortBond) {
    utstring_printf(pStr, "bond,");
  }
  if ((e->sinfo.port_type & PortVxlanSif) == PortVxlanSif) {
    utstring_printf(pStr, "vxlan-sif,");
  }
  if ((e->sinfo.port_type & PortVti) == PortVti) {
    utstring_printf(pStr, "vti,");
  }
  if ((e->sinfo.port_type & PortWg) == PortWg) {
    utstring_printf(pStr, "wg,");
  }
  if ((e->sinfo.port_type & PortPropUpp) == PortPropUpp) {
    utstring_printf(pStr, "upp,");
  }
  if ((e->sinfo.port_type & PortPropPol) == PortPropPol) {
    utstring_printf(pStr, "pol%d,", e->sinfo.port_pol_num);
  }
  if ((e->sinfo.port_type & PortPropSpan) == PortPropSpan) {
    utstring_printf(pStr, "mirr%d,", e->sinfo.port_mir_num);
  }
  if ((e->sinfo.port_type & PortVxlanBr) == PortVxlanBr) {
    utstring_printf(pStr, "vxlan");
    if (e->sinfo.port_real) {
      utstring_printf(pStr, "(%s)", e->sinfo.port_real->name);
    }
  }

  int l = utstring_len(pStr);
  if (l > 0) {
    if (utstring_body(pStr)[l - 1] == ',') {
      utstring_body(pStr)[l - 1] = 0;
    }
  }

  utstring_printf(s, "%-10s  ether %02x:%02x:%02x:%02x:%02x:%02x  %s\n", "",
                  e->hinfo.mac_addr[0], e->hinfo.mac_addr[1],
                  e->hinfo.mac_addr[2], e->hinfo.mac_addr[3],
                  e->hinfo.mac_addr[4], e->hinfo.mac_addr[5],
                  utstring_body(pStr));

  it.node_walker(utstring_body(s));

  utstring_free(pStr);
  utstring_free(s);
}

void lbrt_ports_2_str(lbrt_ports_h_t *ph, lbrt_iter_intf_t it) {
  lbrt_port_t *p, *tmp;
  HASH_ITER(hh_by_name, ph->port_s_map, p, tmp) { __lbrt_port_2_str(p, it); }
}

bool lbrt_port_is_leaf_port(lbrt_port_t *port) {
  if ((port->sinfo.port_type & (PortReal | PortBond | PortVti | PortWg)) == 0) {
    return false;
  }
  return true;
}

bool lbrt_port_is_slave_port(lbrt_port_t *port) {
  if ((port->sinfo.port_type & (PortVlanSif | PortBondSif)) == 0) {
    return false;
  }
  return true;
}

bool lbrt_port_is_l3_tun_port(lbrt_port_t *port) {
  if ((port->sinfo.port_type & (PortVti | PortWg | PortIPTun)) == 0) {
    return false;
  }
  return true;
}

int lbrt_port_datapath(lbrt_port_t *port, enum lbrt_dp_work work) { return 0; }