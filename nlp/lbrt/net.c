#include <lbrt/net.h>

struct lbrt_net_meta mh;

void lock(void) { printf("##LOCK##\n"); }

void unlock(void) { printf("#UNLOCK#\n"); }

void lbrt_net_init(void) {
  memset(&mh, 0, sizeof(lbrt_net_meta_h));
  mh.zn = lbrt_zone_h_alloc();
  int ret = lbrt_zone_add(mh.zn, ROOT_ZONE);
  flb_log(LOG_LEVEL_DEBUG, "lbrt_zone_add ret=[%d]", ret);
  mh.zr = lbrt_zone_find(mh.zn, ROOT_ZONE);
  mh.mtx = calloc(1, sizeof(lbrt_mutex_t));
  mh.mtx->lock = lock;
  mh.mtx->unlock = unlock;
}

void lbrt_net_uninit(void) {
  if (mh.zn) {
    lbrt_zone_h_free(mh.zn);
  }
  if (mh.zr) {
    mh.zr = NULL;
  }
}

int lbrt_net_port_add(api_port_mod_t *pm) {
  mh.mtx->lock();

  lbrt_port_hw_info_t hwi;
  memset(&hwi, 0, sizeof(hwi));
  memcpy(hwi.mac_addr, pm->mac_addr, ETH_ALEN);
  hwi.link = pm->link;
  hwi.state = pm->state;
  hwi.mtu = pm->mtu;
  if (pm->master)
    memcpy(hwi.master, pm->master, strlen(pm->master));
  if (pm->real)
    memcpy(hwi.real, pm->real, strlen(pm->real));
  hwi.tun_id = pm->tun_id;
  if (pm->tun_src)
    parse_ip(pm->tun_src, &hwi.tun_src);
  if (pm->tun_dst)
    parse_ip(pm->tun_dst, &hwi.tun_dst);

  lbrt_port_layer2_info_t l2i;
  memset(&l2i, 0, sizeof(l2i));
  l2i.is_p_vid = false;
  l2i.vid = 0;
  int ret = lbrt_port_add(mh.zr->ports, pm->dev, pm->link_index, pm->link_type,
                          ROOT_ZONE, &hwi, &l2i);
  mh.mtx->unlock();
  return ret;
}
int lbrt_net_port_del(api_port_mod_t *pm) {
  // todo
  return 0;
}