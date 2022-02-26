#include <stdio.h>
#include <stdlib.h>

#include "../utils/common.h"
#include "../utils/pkt-ops.h"
// #define VALUE_TYPE U32
#define TYPE uint32_t
#define TYPE_STR U32
#define TYPED_NAME(x) u32_##x
#include "../utils/dleft-hash.h"
#undef TYPE
#undef TYPE_STR
#undef TYPED_NAME

#define HT_SIZE_MON (0.5 * 1024 * 1024)
static u32_dleft_meta_t ht_meta_monitor;
static uint32_t pkt_cnt_monitor = 0;

int monitoring_init() {
  if (-1 == u32_dleft_init("monitoring", HT_SIZE_MON, &ht_meta_monitor)) {
    printf("bootmemory allocation error\n");
    return 0;
  }
  srandom((unsigned)time(NULL));
  printf("monitoring init done!\n");
  return 0;
}
void monitoring(uint8_t *pkt_ptr) {
  swap_mac_addr(pkt_ptr);

  five_tuple_t flow;
  get_five_tuple(pkt_ptr, &flow);
  u32_dleft_add_value(&ht_meta_monitor, flow, 1);

  pkt_cnt_monitor++;
}
void monitoring_destroy() { u32_dleft_destroy(&ht_meta_monitor); }