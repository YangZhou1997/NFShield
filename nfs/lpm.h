#include <stdio.h>
#include <stdlib.h>

#include "../utils/common.h"
#include "../utils/lpm-algo.h"
#include "../utils/pkt-ops.h"

static iplookup_t iplookup;
static uint32_t gate_count[GATE_NUM];
static uint32_t pkt_cnt_lpm = 0;

int lpm_init() {
  srandom(0xCFF32987);
#define RAND8 (random() & 0xFF)
#define TOIP(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | d)

  for (int i = 0; i < 16000; i++) {
    uint32_t ip = TOIP(RAND8, RAND8, RAND8, RAND8);
    uint16_t gate = RAND8;
    lpm_insert(&iplookup, ip, 32, gate);
  }
  lpm_construct_table(&iplookup);
  printf("lpm init done!\n");
  return 0;
}

void lpm(uint8_t *pkt_ptr) {
  swap_mac_addr(pkt_ptr);
  uint32_t srcip = get_src_ip(pkt_ptr);
  uint16_t gate = lpm_lookup(&iplookup, srcip);
  gate_count[gate]++;
  pkt_cnt_lpm++;
}

void lpm_destroy() { return; }
