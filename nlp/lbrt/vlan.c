#include <lbrt/vlan.h>

extern struct lbrt_net_meta mh;

lbrt_vlans_h_t *lbrt_vlans_h_new(lbrt_zone_t *zone) {
  lbrt_vlans_h_t *vh;
  vh = calloc(1, sizeof(*vh));
  if (!vh) {
    return NULL;
  }
  vh->zone = zone;
  return vh;
}

void lbrt_vlans_h_free(lbrt_vlans_h_t *vh) {
  if (!vh)
    return;
  free(vh);
}

bool lbrt_vlan_valid(__u32 vlan_id) {
  return vlan_id > 0 && vlan_id < MaximumVlans - 1;
}

int lbrt_vlan_add(lbrt_vlans_h_t *vh, __u32 vlan_id, char *name, char *zone,
                  __u32 osid, lbrt_port_hw_info_t *hwi) {
  if (!lbrt_vlan_valid(vlan_id)) {
    return VLAN_RANGE_ERR;
  }

  lbrt_vlan_t *vlan = vh->vlan_map[vlan_id];
  if (vlan && vlan->created)
    return VLAN_EXISTS_ERR;

  int ret = lbrt_zone_br_add(mh.zn, name, zone);
  if (ret < 0)
    return VLAN_EXISTS_ERR;

  lbrt_port_layer2_info_t l2i;
  memset(&l2i, 0, sizeof(lbrt_port_layer2_info_t));
  l2i.is_p_vid = false;
  l2i.vid = vlan_id;
  ret = lbrt_port_add(vh->zone->ports, name, osid, PortVlanBr, zone, hwi, &l2i);
  if (ret < 0) {
    flb_log(LOG_LEVEL_ERR, "Vlan bridge interface not created %d", ret);
    lbrt_zone_br_del(mh.zn, name);
    return VLAN_ADD_BRP_ERR;
  }

  vlan = calloc(1, sizeof(lbrt_vlan_t));
  vlan->vlan_id = vlan_id;
  memcpy(vlan->name, name, strlen(name));
  memcpy(vlan->zone, zone, strlen(zone));
  vlan->created = true;

  vh->vlan_map[vlan_id] = vlan;

  flb_log(LOG_LEVEL_INFO, "vlan %d bd created", vlan_id);

  return 0;
}

int lbrt_vlan_del(lbrt_vlans_h_t *vh, __u32 vlan_id) {
  if (!lbrt_vlan_valid(vlan_id)) {
    return VLAN_RANGE_ERR;
  }

  lbrt_vlan_t *vlan = vh->vlan_map[vlan_id];
  if (!vlan || !vlan->created)
    return VLAN_NOT_EXIST_ERR;

  if (vlan->num_tag_ports != 0 || vlan->num_un_tag_ports != 0) {
    return VLAN_MP_EXIST_ERR;
  }

  lbrt_zone_br_del(mh.zn, vlan->name);

  lbrt_port_del(vh->zone->ports, vlan->name, PortVlanBr);
  lbrt_vlan_datapath(vlan, DP_STATSCLR);

  vh->vlan_map[vlan_id] = NULL;
  free(vlan);

  flb_log(LOG_LEVEL_INFO, "vlan %d bd deleted", vlan_id);

  return 0;
}

int lbrt_vlan_port_add(lbrt_vlans_h_t *vh, __u32 vlan_id, char *port_name,
                       bool tagged) {
  if (!lbrt_vlan_valid(vlan_id)) {
    return VLAN_RANGE_ERR;
  }

  lbrt_vlan_t *v = vh->vlan_map[vlan_id];
  if (!v || !v->created) {
    // FIXME : Do we create implicitly here
    flb_log(LOG_LEVEL_ERR, "Vlan not created");
    return VLAN_NOT_EXIST_ERR;
  }

  lbrt_port_t *p = lbrt_port_find_by_name(vh->zone->ports, port_name);
  if (!p) {
    flb_log(LOG_LEVEL_ERR, "Phy port not created %s", port_name);
    return VLAN_PORT_PHY_ERR;
  }

  if (tagged) {
    if ((p->sinfo.port_type & PortVxlanBr) == PortVxlanBr) {
      return VLAN_PORT_TAGGED_ERR;
    }
    if (v->tagged_ports[p->port_no]) {
      return VLAN_PORT_EXIST_ERR;
    }

    __u32 osid = 4000 + (vlan_id * MaxRealInterfaces) + p->port_no;
    char memb_port_name[IF_NAMESIZE + 8];
    memset(memb_port_name, 0, IF_NAMESIZE + 8);
    snprintf(memb_port_name, IF_NAMESIZE + 8, "%s.%d", port_name, vlan_id);

    lbrt_port_hw_info_t hinfo;
    memcpy(&hinfo, &p->hinfo, sizeof(hinfo));
    memcpy(hinfo.real, p->name, strlen(p->name));
    memcpy(hinfo.master, v->name, strlen(v->name));

    lbrt_port_layer2_info_t l2i;
    l2i.is_p_vid = false;
    l2i.vid = vlan_id;

    int ret = lbrt_port_add(vh->zone->ports, memb_port_name, osid, PortVlanSif,
                            v->zone, &hinfo, &l2i);
    if (ret == 0) {
      lbrt_port_t *tp = lbrt_port_find_by_name(vh->zone->ports, memb_port_name);
      if (!tp) {
        return VLAN_PORT_CREATE_ERR;
      }
      v->tagged_ports[p->port_no] = tp;
      v->num_tag_ports++;
    } else {
      return VLAN_PORT_CREATE_ERR;
    }

  } else {
    if (v->un_tagged_ports[p->port_no]) {
      return VLAN_PORT_EXIST_ERR;
    }
    lbrt_port_hw_info_t hinfo;
    memcpy(&hinfo, &p->hinfo, sizeof(hinfo));
    memcpy(hinfo.master, v->name, strlen(v->name));

    lbrt_port_layer2_info_t l2i;
    l2i.is_p_vid = true;
    l2i.vid = vlan_id;

    int ret = lbrt_port_add(vh->zone->ports, port_name, p->sinfo.osid,
                            PortVlanSif, v->zone, &hinfo, &l2i);
    if (ret == 0) {
      v->un_tagged_ports[p->port_no] = p;
      v->num_un_tag_ports++;
    } else {
      return VLAN_PORT_CREATE_ERR;
    }
  }

  return 0;
}

int lbrt_vlan_port_del(lbrt_vlans_h_t *vh, __u32 vlan_id, char *port_name,
                       bool tagged) {
  if (!lbrt_vlan_valid(vlan_id)) {
    return VLAN_RANGE_ERR;
  }

  lbrt_vlan_t *v = vh->vlan_map[vlan_id];
  if (!v || !v->created)
    return VLAN_NOT_EXIST_ERR;

  lbrt_port_t *p = lbrt_port_find_by_name(vh->zone->ports, port_name);
  if (!p) {
    return VLAN_PORT_PHY_ERR;
  }

  if (tagged) {
    lbrt_port_t *tp = v->tagged_ports[p->port_no];
    if (!tp) {
      return VLAN_NO_PORT_ERR;
    }
    char memb_port_name[IF_NAMESIZE + 8];
    memset(memb_port_name, 0, IF_NAMESIZE + 8);
    snprintf(memb_port_name, IF_NAMESIZE + 8, "%s.%d", port_name, vlan_id);
    lbrt_port_del(vh->zone->ports, memb_port_name, PortVlanSif);
    v->tagged_ports[p->port_no] = NULL;
    v->num_tag_ports--;
  } else {
    lbrt_port_del(vh->zone->ports, port_name, PortVlanSif);
    v->un_tagged_ports[p->port_no] = NULL;
    v->num_un_tag_ports--;
  }

  return 0;
}

void lbrt_vlan_destruct_all(lbrt_vlans_h_t *vh) {
  lbrt_vlan_t *v = NULL;
  lbrt_port_t *vp = NULL, *mp = NULL;
  for (int i = 0; i < MaximumVlans; i++) {
    v = vh->vlan_map[i];
    if (v && v->created) {
      vp = lbrt_port_find_by_name(vh->zone->ports, v->name);
      if (!vp)
        continue;

      for (int p = 0; p < MaxInterfaces; p++) {
        mp = v->tagged_ports[p];
        if (mp) {
          lbrt_vlan_port_del(vh, i, mp->name, true);
        }
      }

      for (int p = 0; p < MaxInterfaces; p++) {
        mp = v->un_tagged_ports[p];
        if (mp) {
          lbrt_vlan_port_del(vh, i, mp->name, false);
        }
      }

      lbrt_vlan_del(vh, i);
    }
  }
}

void lbrt_vlans_2_str(lbrt_vlans_h_t *vh, lbrt_iter_intf_t it) {
  lbrt_vlan_t *v = NULL;
  lbrt_port_t *vp = NULL, *mp = NULL;
  UT_string *s;
  utstring_new(s);

  for (int i = 0; i < MaximumVlans; i++) {
    utstring_clear(s);

    v = vh->vlan_map[i];
    if (v && v->created) {
      vp = lbrt_port_find_by_name(vh->zone->ports, v->name);
      if (vp) {
        utstring_printf(s, "%-10s: ", vp->name);
      } else {
        flb_log(LOG_LEVEL_ERR, "VLan %s not found", v->name);
        continue;
      }

      utstring_printf(s, "Tagged-   ");
      for (int p = 0; p < MaxInterfaces; p++) {
        mp = v->tagged_ports[p];
        if (mp) {
          utstring_printf(s, "%s,", mp->name);
        }
      }

      int l = utstring_len(s);
      if (l > 0) {
        if (utstring_body(s)[l - 1] == ',') {
          utstring_body(s)[l - 1] = 0;
        }
      }

      utstring_printf(s, "\n%22s", "UnTagged- ");
      for (int p = 0; p < MaxInterfaces; p++) {
        mp = v->un_tagged_ports[p];
        if (mp) {
          utstring_printf(s, "%s,", mp->name);
        }
      }

      l = utstring_len(s);
      if (l > 0) {
        if (utstring_body(s)[l - 1] == ',') {
          utstring_body(s)[l - 1] = 0;
        }
      }

      utstring_printf(s, "\n");

      it.node_walker(utstring_body(s));
    }
  }

  utstring_free(s);
}

void lbrt_vlans_ticker(lbrt_vlans_h_t *vh) {
  // TODO
}

int lbrt_vlan_datapath(lbrt_vlan_t *vlan, enum lbrt_dp_work work) { return 0; }