#include <stdio.h>
#include <stdlib.h>

#include "../utils/common.h"
#include "../utils/maglev-algo.h"
#include "../utils/pkt-ops.h"

#define M 65537
#define N 3
static uint32_t backend_ip[N];
static uint8_t backend_status[N];
static uint32_t pkt_cnt_maglev = 0;

static maglev_meta_t mag_meta;
int maglev_init() {
  srandom(0xCFF32987);
  for (int i = 0; i < N; i++) {
    backend_ip[i] = random();
    backend_status[i] = HEALTHY;
  }
  maglev_init_inner("maglev", M, N, backend_ip, backend_status, &mag_meta);
  printf("maglev init done!\n");
  return 0;
}

void maglev(uint8_t *pkt_ptr) {
  swap_mac_addr(pkt_ptr);

  five_tuple_t key;
  get_five_tuple(pkt_ptr, &key);
  uint32_t lookup_value;
  lookup_value = maglev_get_backend(&mag_meta, key);
  set_dst_ip(pkt_ptr, lookup_value);

  pkt_cnt_maglev++;
}

void maglev_destroy() { maglev_destory_inner(&mag_meta); }
