#include <lbrt/layer3.h>

extern struct lbrt_net_meta mh;

UT_icd lbrt_ifa_ent_icd = {sizeof(lbrt_ifa_ent_t), NULL, NULL, NULL};

lbrt_ifa_t *lbrt_ifa_new() {
  lbrt_ifa_t *ifa = calloc(1, sizeof(lbrt_ifa_t));
  utarray_new(ifa->ifas, &lbrt_ifa_ent_icd);
  return ifa;
}

void lbrt_ifa_free(lbrt_ifa_t *ifa) {
  if (!ifa)
    return;
  utarray_free(ifa->ifas);
  free(ifa);
}

lbrt_l3_h_t *lbrt_l3_h_new(lbrt_zone_t *zone) {
  lbrt_l3_h_t *l3h;
  l3h = calloc(1, sizeof(*l3h));
  if (!l3h) {
    return NULL;
  }
  l3h->zone = zone;
  return l3h;
}

void lbrt_l3_h_free(lbrt_l3_h_t *l3h) {
  if (!l3h)
    return;
  lbrt_ifa_t *p, *tmp;
  HASH_ITER(hh, l3h->ifa_map, p, tmp) {
    HASH_DEL(l3h->ifa_map, p);
    free(p);
  }
  free(l3h);
}

lbrt_ifa_t *lbrt_ifa_find(lbrt_l3_h_t *l3h, const char *obj) {
  lbrt_ifa_t *ifa = NULL;
  lbrt_ifa_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(&key.obj, obj, strlen(obj));
  HASH_FIND(hh, l3h->ifa_map, &key, sizeof(key), ifa);
  return ifa;
}

int lbrt_ifa_add(lbrt_l3_h_t *l3h, const char *obj, const char *cidr) {
  bool sec = false;
  char addr[IF_ADDRSIZE];
  ip_t ip;
  ip_net_t network;
  bool ok = parse_ip_net(cidr, &network, &ip);
  if (!ok) {
    return L3_ADDR_ERR;
  } else {
    ip_ntoa(&ip, addr);
  }

  __u32 ifObjID = 0;
  lbrt_port_t *pObj = lbrt_port_find_by_name(l3h->zone->ports, obj);
  if (pObj) {
    ifObjID = pObj->sinfo.os_id;
  }

  lbrt_ifa_t *ifa = lbrt_ifa_find(l3h, obj);

  if (!ifa) {
    ifa = lbrt_ifa_new();
    memcpy(ifa->ifa_key.obj, obj, strlen(obj));
    ifa->zone = l3h->zone;

    lbrt_ifa_ent_t ifaEnt;
    memset(&ifaEnt, 0, sizeof(ifaEnt));
    memcpy(&ifaEnt.ifa_addr, &ip, sizeof(ip_t));
    memcpy(&ifaEnt.ifa_net, &network, sizeof(ip_net_t));
    utarray_push_back(ifa->ifas, &ifaEnt);
    HASH_ADD(hh, l3h->ifa_map, ifa_key, sizeof(lbrt_ifa_key_t), ifa);

    lbrt_rt_attr_t ra;
    memset(&ra, 0, sizeof(ra));
    ra.ifi = ifObjID;
    int ret = lbrt_rt_add(mh.zr->rt, cidr, ROOT_ZONE, &ra, 0, NULL);

    if (ret < 0) {
      flb_log(LOG_LEVEL_DEBUG, "ifa add - %s:%s self-rt error", addr, obj);
      return L3_ADDR_ERR;
    }

    lbrt_ifa_datapath(ifa, DP_CREATE);

    flb_log(LOG_LEVEL_DEBUG, "ifa added %s:%s", addr, obj);

    return 0;
  }

  for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
       ifaEnt != NULL;
       (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
    if (memcmp(&ip, &ifaEnt->ifa_addr, sizeof(ip_t)) == 0) {
      flb_log(LOG_LEVEL_DEBUG, "ifa add - exists %s:%s", addr, obj);
      return L3_ADDR_ERR;
    }

    // if network part of an added ifa is equal to previously
    // existing ifa, then it is considered a secondary ifa
    if (memcmp(&network, &ifaEnt->ifa_net, sizeof(ip_net_t)) == 0) {
      __u8 pfxSz1 = ifaEnt->ifa_net.mask;
      __u8 pfxSz2 = network.mask;
      if (pfxSz1 == pfxSz2) {
        sec = true;
      }
    }
  }

  lbrt_ifa_ent_t ifaEnt;
  memset(&ifaEnt, 0, sizeof(ifaEnt));
  memcpy(&ifaEnt.ifa_addr, &ip, sizeof(ip_t));
  memcpy(&ifaEnt.ifa_net, &network, sizeof(ip_net_t));
  ifaEnt.secondary = sec;
  utarray_push_back(ifa->ifas, &ifaEnt);

  lbrt_rt_attr_t ra;
  memset(&ra, 0, sizeof(ra));
  ra.ifi = ifObjID;
  int ret = lbrt_rt_add(mh.zr->rt, cidr, ROOT_ZONE, &ra, 0, NULL);
  if (ret < 0) {
    flb_log(LOG_LEVEL_DEBUG, " - %s:%s self-rt error", addr, obj);
    return L3_ADDR_ERR;
  }

  lbrt_ifa_datapath(ifa, DP_CREATE);

  flb_log(LOG_LEVEL_DEBUG, "ifa added %s:%s", addr, obj);

  return 0;
}

int lbrt_ifa_del(lbrt_l3_h_t *l3h, const char *obj, const char *cidr) {
  bool found = false;
  char addr[IF_ADDRSIZE];
  ip_t ip;
  ip_net_t network;
  bool ok = parse_ip_net(cidr, &network, &ip);
  if (!ok) {
    flb_log(LOG_LEVEL_ERR, "ifa delete - malformed cidr %s:%s", cidr, obj);
    return L3_ADDR_ERR;
  } else {
    ip_ntoa(&ip, addr);
  }

  lbrt_ifa_t *ifa = lbrt_ifa_find(l3h, obj);

  if (!ifa) {
    flb_log(LOG_LEVEL_ERR, "ifa delete - no such %s:%s", addr, obj);
    return L3_ADDR_ERR;
  }

  for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
       ifaEnt != NULL;
       (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
    if (memcmp(&ip, &ifaEnt->ifa_addr, sizeof(ip_t)) == 0) {
      if (memcmp(&network, &ifaEnt->ifa_net, sizeof(ip_net_t)) == 0) {
        __u8 pfxSz1 = ifaEnt->ifa_net.mask;
        __u8 pfxSz2 = network.mask;
        if (pfxSz1 == pfxSz2) {
          flb_log(LOG_LEVEL_DEBUG, "ifa ent deleted %s:%s", cidr, obj);
          __u32 idx = utarray_eltidx(ifa->ifas, ifaEnt);
          utarray_erase(ifa->ifas, idx, 1);
          found = true;
          break;
        }
      }
    }
  }

  if (found) {
    // delete self-routes related to this ifa
    int ret = lbrt_rt_del(mh.zr->rt, cidr, ROOT_ZONE);
    if (ret < 0) {
      flb_log(LOG_LEVEL_ERR, "ifa delete %s:%s self-rt error", addr, obj);
      // ontinue after logging error because there is noway to fallback
    }
    if (utarray_len(ifa->ifas) == 0) {
      HASH_DELETE(hh, l3h->ifa_map, ifa);
      lbrt_ifa_datapath(ifa, DP_REMOVE);
      lbrt_ifa_free(ifa);
      flb_log(LOG_LEVEL_DEBUG, "ifa deleted %s:%s", addr, obj);
    }
    return 0;
  }

  flb_log(LOG_LEVEL_DEBUG, "ifa delete - no such %s:%s", addr, obj);

  return L3_ADDR_ERR;
}

int lbrt_ifa_del_all(lbrt_l3_h_t *l3h, const char *obj) {
  lbrt_ifa_t *ifa = lbrt_ifa_find(l3h, obj);
  if (ifa) {
    bool last = false;
    char ip_str[IF_ADDRSIZE];
    char cidr[IF_CIDRSIZE];
    lbrt_ifa_ent_t *ifaFirst = NULL, *ifaLast = NULL;
    while ((ifaFirst = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas)) != NULL) {
      ifaLast = (lbrt_ifa_ent_t *)utarray_back(ifa->ifas);
      if (ifaFirst == ifaLast) {
        last = true;
      }
      ip_ntoa(&ifaFirst->ifa_addr, ip_str);
      memset(cidr, 0, IF_CIDRSIZE);
      snprintf(cidr, IF_CIDRSIZE, "%s/%d", ip_str, ifaFirst->ifa_net.mask);
      lbrt_ifa_del(l3h, obj, cidr);
      if (last)
        break;
    }
  }
  return 0;
}

int lbrt_ifa_select(lbrt_l3_h_t *l3h, const char *obj, ip_t *addr, bool findAny,
                    ip_t *out_sip, char *out_dev) {
  lbrt_ifa_t *ifa = lbrt_ifa_find(l3h, obj);
  if (!ifa) {
    return L3_OBJ_ERR;
  }

  memset(out_sip, 0, sizeof(ip_t));
  memset(out_dev, 0, IF_NAMESIZE);

  for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
       ifaEnt != NULL;
       (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
    if (ifaEnt->secondary)
      continue;
    if (addr->f.v6 && ifaEnt->ifa_net.ip.f.v4)
      continue;
    if (ip_net_contains(&ifaEnt->ifa_net, addr)) {
      memcpy(out_dev, obj, strlen(obj));
      memcpy(out_sip, &ifaEnt->ifa_addr, sizeof(ip_t));
      return 0;
    }
  }

  if (!findAny)
    return L3_ADDR_ERR;

  // Select first IP
  if (utarray_len(ifa->ifas) > 0) {
    lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
    memcpy(out_dev, obj, strlen(obj));
    memcpy(out_sip, &ifaEnt->ifa_addr, sizeof(ip_t));
    return 0;
  }

  return L3_ADDR_ERR;
}

int lbrt_ifa_select_any(lbrt_l3_h_t *l3h, ip_t *addr, bool findAny,
                        ip_t *out_sip, char *out_dev) {
  int ret;
  bool v6 = false;
  char *ifObj = "";
  ip_t *firstIP = NULL;
  char *firstIfObj = NULL;
  char addr_str[IF_ADDRSIZE];
  ip_ntoa(addr, addr_str);

  ip_net_t ipnet;
  lbrt_trie_data_t trie_data;
  if (addr->f.v4) {
    ret = lbrt_trie_find(l3h->zone->rt->trie4, addr_str, &ipnet, &trie_data);
  } else {
    ret = lbrt_trie_find(l3h->zone->rt->trie6, addr_str, &ipnet, &trie_data);
    v6 = true;
  }

  if (ret == 0) {
    if (trie_data.f.osid) {
      lbrt_port_t *p =
          lbrt_port_find_by_osid(l3h->zone->ports, trie_data.v.osid);
      if (p) {
        ifObj = p->name;
      }
    } else if (trie_data.f.neigh) {
      lbrt_neigh_t *n = (lbrt_neigh_t *)trie_data.v.neigh;
      ifObj = n->o_if_port->name;
    }
  }

  if (strcmp(ifObj, "") != 0 && strcmp(ifObj, "lo") != 0) {
    return lbrt_ifa_select(l3h, ifObj, addr, findAny, out_sip, out_dev);
  }

  memset(out_sip, 0, sizeof(ip_t));
  memset(out_dev, 0, IF_NAMESIZE);

  lbrt_ifa_t *ifa, *ifa_tmp = NULL;
  HASH_ITER(hh, l3h->ifa_map, ifa, ifa_tmp) {
    if (strcmp(ifa->ifa_key.obj, "lo") == 0)
      continue;

    for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
         ifaEnt != NULL;
         (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
      if (ifaEnt->secondary)
        continue;
      if (v6 && ifaEnt->ifa_net.ip.f.v4)
        continue;
      if (ip_net_contains(&ifaEnt->ifa_net, addr)) {
        memcpy(out_dev, ifa->ifa_key.obj, strlen(ifa->ifa_key.obj));
        memcpy(out_sip, &ifaEnt->ifa_addr, sizeof(ip_t));
        return 0;
      }

      if (!firstIP) {
        firstIP = &ifaEnt->ifa_addr;
        firstIfObj = ifa->ifa_key.obj;
      }
    }
  }

  if (!findAny)
    return L3_ADDR_ERR;

  if (firstIP) {
    memcpy(out_dev, firstIfObj, strlen(firstIfObj));
    memcpy(out_sip, firstIP, sizeof(ip_t));
    return 0;
  }

  return L3_ADDR_ERR;
}

void lbrt_ifa_2_str(lbrt_ifa_t *ifa, lbrt_iter_intf_t it) {
  char addr[IF_ADDRSIZE];
  char str[70];
  for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
       ifaEnt != NULL;
       (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
    ip_ntoa(&ifaEnt->ifa_addr, addr);
    memset(str, 0, 70);
    if (ifaEnt->secondary) {
      snprintf(str, 70, "%s/%d - %s", addr, ifaEnt->ifa_net.mask, "Secondary");
    } else {
      snprintf(str, 70, "%s/%d - %s", addr, ifaEnt->ifa_net.mask, "Primary");
    }
    it.node_walker(str);
  }
}

void lbrt_ifas_2_str(lbrt_l3_h_t *l3h, lbrt_iter_intf_t it) {
  lbrt_ifa_t *ifa, *tmp;
  HASH_ITER(hh, l3h->ifa_map, ifa, tmp) { lbrt_ifa_2_str(ifa, it); }
}

bool lbrt_ifa_mk_str(lbrt_ifa_t *ifa, bool v4, char out_str[56]) {
  bool ret = false;
  char addr[IF_ADDRSIZE];
  for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
       ifaEnt != NULL;
       (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
    if (!v4 && ifaEnt->ifa_addr.f.v4)
      continue;
    if (v4 && ifaEnt->ifa_addr.f.v6)
      continue;

    char *flagStr = NULL;
    if (ifaEnt->secondary) {
      flagStr = "S";
    } else {
      flagStr = "P";
    }

    ip_ntoa(&ifaEnt->ifa_addr, addr);
    memset(out_str, 0, 56);
    snprintf(out_str, 56, "%s/%d - %s", addr, ifaEnt->ifa_net.mask, flagStr);
    ret = true;
    break;
  }
  return ret;
}

bool lbrt_ifa_obj_mk_str(lbrt_l3_h_t *l3h, const char *obj, bool v4,
                         char out_str[56]) {
  lbrt_ifa_t *ifa = lbrt_ifa_find(l3h, obj);
  if (ifa) {
    return lbrt_ifa_mk_str(ifa, v4, out_str);
  }
  return false;
}

void api_ip_addr_get_dtor(void *_elt) {
  api_ip_addr_get_t *elt = (api_ip_addr_get_t *)_elt;
  if (elt->dev) {
    flb_log(LOG_LEVEL_TRACE, "free elt->dev=[%s]", elt->dev);
    free(elt->dev);
  }
  if (elt->ip_cnt > 0) {
    flb_log(LOG_LEVEL_TRACE, "free elt->ip[%d]", elt->ip_cnt);
    for (int i = 0; i < elt->ip_cnt; i++) {
      flb_log(LOG_LEVEL_TRACE, "free elt->ip[%d]=[%s]", i, elt->ip[i]);
      free(elt->ip[i]);
    }
    free(elt->ip);
  }
}

UT_icd api_ip_addr_get_icd = {sizeof(api_ip_addr_get_t), NULL, NULL,
                              api_ip_addr_get_dtor};

UT_array *lbrt_ifa_get(lbrt_l3_h_t *l3h) {
  UT_array *ipgets;
  utarray_new(ipgets, &api_ip_addr_get_icd);

  char ipaddr[IF_ADDRSIZE];

  lbrt_ifa_t *ifa, *ifa_tmp = NULL;
  HASH_ITER(hh, l3h->ifa_map, ifa, ifa_tmp) {
    api_ip_addr_get_t *ipget = calloc(1, sizeof(api_ip_addr_get_t));
    ipget->dev = (char *)calloc(1, IF_NAMESIZE);
    memcpy(ipget->dev, ifa->ifa_key.obj, IF_NAMESIZE);
    ipget->sync = ifa->sync;
    ipget->ip_cnt = utarray_len(ifa->ifas);
    ipget->ip = (char **)calloc(1, sizeof(char *) * ipget->ip_cnt);
    int index = 0;

    for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
         ifaEnt != NULL;
         (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
      ipget->ip[index] = (char *)calloc(1, IF_CIDRSIZE);
      ip_ntoa(&ifaEnt->ifa_addr, ipaddr);
      snprintf(ipget->ip[index], IF_CIDRSIZE, "%s/%d", ipaddr,
               ifaEnt->ifa_net.mask);
      index++;
    }

    utarray_push_back(ipgets, ipget);
  }
  return ipgets;
}

int lbrt_ifa_datapath(lbrt_ifa_t *ifa, enum lbrt_dp_work work) { return 0; }