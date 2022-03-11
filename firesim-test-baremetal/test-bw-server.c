#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/encoding.h"
#include "../utils/icenic_util.h"
#include "../utils/mmio.h"
#include "common.h"

uint64_t in_packets[NPACKETS][PACKET_WORDS];
int total_req = 0;
int total_comp = 0;
char inflight[NPACKETS];
int print_cnt = 0;

static inline void process_loop(void) {
  uint32_t counts, recv_req, recv_comp;
  static int req_id = 0, comp_id = 0;
  int len;

  counts = nic_counts();
  recv_req = (counts >> NIC_COUNT_RECV_REQ) & 0xff;
  recv_comp = (counts >> NIC_COUNT_RECV_COMP) & 0xff;
  if (recv_req != 0 || recv_comp != 0) {
    printf("%d vs. %d\n", recv_req, recv_comp);
  }

  for (int i = 0; i < recv_comp; i++) {
    len = nic_complete_recv();
    if (len != PACKET_WORDS * sizeof(uint64_t)) {
      printf("Incorrectly sized packet\n");
      abort();
    }
    inflight[comp_id] = 0;
    comp_id = (comp_id + 1) % NPACKETS;
    total_comp++;
    uint64_t* packet = in_packets[comp_id];
    if ((print_cnt++) % 10 == 0) {
      printf("recv_comp: %lx\n", packet[2]);
    }
  }

  for (int i = 0; i < recv_req; i++) {
    if (inflight[req_id]) break;
    nic_post_recv_no_check((uint64_t)in_packets[req_id]);
    inflight[req_id] = 1;
    req_id = (req_id + 1) % NPACKETS;
    total_req++;
  }
}

void thread_entry(int cid, int nc) {
  if (cid != 0) {
    return;
  }
  uint64_t cycle;

  memset(inflight, 0, NPACKETS);

  do {
    process_loop();
    cycle = rdcycle();
  } while (1);

  printf("cycles: %lu, completed: %d\n", cycle, total_comp);

  return 0;
}