#include <stdio.h>
#include <stdlib.h>

#include "../config.h"
#include "../utils/common.h"
#include "../utils/pkt-ops.h"

static uint32_t pkt_cnt_mem_test = 0;
static uint8_t array[MEM_TEST_TOTAL_BYTES];

int mem_test_init() {
  srandom((unsigned)time(NULL));
  printf("mem_test init done!\n");
  return 0;
}
void mem_test(uint8_t *pkt_ptr) {
  swap_mac_addr(pkt_ptr);

  uint32_t start =
      random() % (MEM_TEST_TOTAL_BYTES - MEM_TEST_ACCESS_BYTES_PER_PKT);
  for (uint32_t i = start;
       i < start + MEM_TEST_ACCESS_BYTES_PER_PKT - sizeof(uint32_t);
       i += sizeof(uint32_t)) {
    *(uint32_t *)(array + i) += 1;
  }

  pkt_cnt_mem_test++;
}
void mem_test_destroy() { return; }