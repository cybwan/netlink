#include <nlp.h>

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
  memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

  while (RTA_OK(rta, len)) {
    if (rta->rta_type <= max) {
      tb[rta->rta_type] = rta;
    }

    rta = RTA_NEXT(rta, len);
  }
}