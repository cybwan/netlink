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
  ip_net_t network;
  bool ok = parse_ip_net(cidr, &network);
  if (!ok) {
    return L3_ADDR_ERR;
  } else {
    ip_ntoa(&network.ip, addr);
  }

  __u32 ifObjID = 0;
  lbrt_port_t *pObj = lbrt_port_find_by_name(l3h->zone->ports, obj);
  if (pObj) {
    ifObjID = pObj->sinfo.os_id;
  }

  lbrt_ifa_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(key.obj, obj, strlen(obj));
  lbrt_ifa_t *ifa = lbrt_ifa_find(l3h, obj);

  if (!ifa) {
    ifa = calloc(1, sizeof(lbrt_ifa_t));
    memcpy(ifa->ifa_key.obj, obj, strlen(obj));
    ifa->zone = l3h->zone;

    lbrt_ifa_ent_t ifaEnt;
    memset(&ifaEnt, 0, sizeof(ifaEnt));
    memcpy(&ifaEnt.ifa_addr, &network.ip, sizeof(ip_t));
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

    return 0;
  }

  for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
       ifaEnt != NULL;
       (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
    if (memcmp(&network.ip, &ifaEnt->ifa_addr, sizeof(ip_t)) == 0) {
      char addr[IF_ADDRSIZE];
      ip_ntoa(&network.ip, addr);
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
  memcpy(&ifaEnt.ifa_addr, &network.ip, sizeof(ip_t));
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
  ip_net_t network;
  bool ok = parse_ip_net(cidr, &network);
  if (!ok) {
    flb_log(LOG_LEVEL_ERR, "ifa delete - malformed cidr %s:%s", cidr, obj);
    return L3_ADDR_ERR;
  } else {
    ip_ntoa(&network.ip, addr);
  }

  lbrt_ifa_key_t key;
  memset(&key, 0, sizeof(key));
  memcpy(key.obj, obj, strlen(obj));
  lbrt_ifa_t *ifa = lbrt_ifa_find(l3h, obj);

  if (!ifa) {
    flb_log(LOG_LEVEL_ERR, "ifa delete - no such %s:%s", addr, obj);
    return L3_ADDR_ERR;
  }

  for (lbrt_ifa_ent_t *ifaEnt = (lbrt_ifa_ent_t *)utarray_front(ifa->ifas);
       ifaEnt != NULL;
       (ifaEnt = (lbrt_ifa_ent_t *)utarray_next(ifa->ifas, ifaEnt))) {
    if (memcmp(&network.ip, &ifaEnt->ifa_addr, sizeof(ip_t)) == 0) {
      if (memcmp(&network, &ifaEnt->ifa_net, sizeof(ip_net_t)) == 0) {
        __u8 pfxSz1 = ifaEnt->ifa_net.mask;
        __u8 pfxSz2 = network.mask;
        if (pfxSz1 == pfxSz2) {
          found = true;
        }
      }
    }
  }

  if (found) {
    // delete self-routes related to this ifa
    int ret = lbrt_rt_del(mh.zr->rt, cidr, ROOT_ZONE);
    if (ret < 0) {
      // tk.LogIt(tk.LogError, "ifa delete %s:%s self-rt error\n",
      // addr.String(), Obj)
      // ontinue after logging error because there is noway to fallback
    }
    if (utarray_len(ifa->ifas) == 0) {
      HASH_DELETE(hh, l3h->ifa_map, ifa);
      lbrt_ifa_datapath(ifa, DP_REMOVE);
      free(ifa);
      flb_log(LOG_LEVEL_DEBUG, "ifa deleted %s:%s", addr, obj);
    }
  }

  flb_log(LOG_LEVEL_DEBUG, "ifa delete - no such %s:%s", addr, obj);

  return L3_ADDR_ERR;
}

int lbrt_ifa_datapath(lbrt_ifa_t *ifa, enum lbrt_dp_work work) { return 0; }