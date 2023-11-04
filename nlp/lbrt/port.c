#include <lbrt/port.h>

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