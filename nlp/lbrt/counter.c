#include <lbrt/counter.h>

struct lbrt_counter *lbrt_counter_alloc(__u64 begin, __u64 length) {
  struct lbrt_counter *c;
  c = calloc(1, sizeof(*c));
  if (!c) {
    return NULL;
  }

  c->counters = calloc(1, sizeof(__u64) * length);
  c->begin = begin;
  c->start = 0;
  c->end = length - 1;
  c->len = length;
  c->cap = length;
  for (__u64 i = 0; i < length; i++) {
    c->counters[i] = i + 1;
  }
  c->counters[length - 1] = ~(__u64)0;
  return c;
}

void lbrt_counter_free(struct lbrt_counter *c) {
  if (!c)
    return;
  if (c->counters)
    free(c->counters);
  free(c);
}

__u64 lbrt_counter_get_counter(struct lbrt_counter *c) {
  if (c->cap <= 0 || c->start == COUNTER_OVERFLOW) {
    return COUNTER_OVERFLOW;
  }
  c->cap--;
  __u64 rid = c->start;
  if (c->start == c->end) {
    c->start = COUNTER_OVERFLOW;
  } else {
    c->start = c->counters[rid];
    c->counters[rid] = COUNTER_OVERFLOW;
  }
  return rid + c->begin;
}

bool lbrt_counter_put_counter(struct lbrt_counter *c, __u64 id) {
  if (id < c->begin || id >= c->begin + c->len) {
    return false;
  }
  __u64 rid = id - c->begin;
  __u64 tmp = c->end;
  c->end = rid;
  c->counters[tmp] = rid;
  c->cap++;
  if (c->start == COUNTER_OVERFLOW) {
    c->start = c->end;
  }
  return true;
}