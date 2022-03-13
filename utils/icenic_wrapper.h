#ifndef __RAW_ICENET_
#define __RAW_ICENET_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "fifo.h"
#include "icenic_util.h"
#include "pkt-header.h"
#include "riscv_util.h"

arch_spinlock_t icenet_lock;

#ifndef NCORES
#define NCORES 4
#endif

// this function is modified from
// https://github.com/l-nic/chipyard/blob/lnic-dev/tests-icenic/icenic.h

/**
 * Send one LNIC pkt to indicate that the system has booted
 * and is ready to start processing pkts.
 * The switch wait for all boot pkts from all cores to arrive
 * before starting the tests.
 */
#define BOOT_PKT_LEN 64
static void nic_boot_pkt(int nf_idx, int num_nfs) {
  unsigned long len = BOOT_PKT_LEN;
  uint8_t buf[BOOT_PKT_LEN];

  struct ether_hdr *eth;
  struct ipv4_hdr *ipv4;

  eth = (void *)(buf + NET_IP_ALIGN);
  ipv4 = (void *)eth + ETH_HEADER_SIZE;

  uint64_t *pkt_bytes = (uint64_t *)buf;
  pkt_bytes[0] = MAC_PKTGEN << 16;
  pkt_bytes[1] =
      MAC_NFTOP | ((uint64_t)htons(CUSTOM_PROTO_BASE + NCORES + nf_idx) << 48);

  ipv4->version_ihl = 0x45;
  ipv4->type_of_service = 0;
  ipv4->total_length = htons(BOOT_PKT_LEN - ETH_HEADER_SIZE);
  ipv4->packet_id = htons((uint16_t)num_nfs);
  ipv4->fragment_offset = htons(0);
  ipv4->time_to_live = 64;
  ipv4->next_proto_id = LNIC_PROTO;
  ipv4->hdr_checksum = 0;  // NOTE: not implemented
  ipv4->src_addr = htoni(0);
  ipv4->dst_addr = htoni(0);

  arch_spin_lock(&icenet_lock);
  nic_send(buf, len);
  arch_spin_unlock(&icenet_lock);
}

// global_pkt_ff holds all pkt_bufs, its pkt_len is meaningless
static fifo_t global_pkt_ff;
// percore_pkt_ffs holds per-core pkt_bufs and pkt_lens
static fifo_t percore_pkt_ffs[NCORES];

int recvfrom_single(int core_id, intptr_t *pkt_buf, int *pkt_len) {
  // first check local fifo
  if (fifo_pop(&percore_pkt_ffs[core_id], pkt_buf, pkt_len)) {
    return 1;
  }

  // popping a new pkt_buf
  intptr_t new_pkt_buf = 0;
  int new_pkt_len = 0;
  bool res = fifo_pop(&global_pkt_ff, &new_pkt_buf, &new_pkt_len);
  if (!res) {
    printf("recvfrom_single global fifo_pop error\n");
    exit(0);
  }

  // receiving a new pkt
  arch_spin_lock(&icenet_lock);
  new_pkt_len = nic_recv((void *)new_pkt_buf);
  arch_spin_unlock(&icenet_lock);

  // identify which core this pkt should head to
  struct ether_hdr *eh_recv =
      (struct ether_hdr *)((uint8_t *)new_pkt_buf + NET_IP_ALIGN);
  int ether_type = (int)htons((eh_recv->ether_type));
  int nf_idx = ether_type - CUSTOM_PROTO_BASE;
  if (nf_idx < 0 || nf_idx >= NCORES) {
    printf("recvfrom_single nf_idx error %d\n", nf_idx);
    exit(0);
  }

  // lucky, getting the right packet
  if (nf_idx == core_id) {
    *pkt_buf = new_pkt_buf;
    *pkt_len = new_pkt_len;
    return 1;
  } else {
    bool res = fifo_push(&percore_pkt_ffs[nf_idx], new_pkt_buf, new_pkt_len);
    if (!res) {
      printf("recvfrom_single local fifo_push error\n");
      exit(0);
    }
    return 0;
  }
}

int sendto_single(int core_id, intptr_t pkt_buf, int pkt_len) {
  arch_spin_lock(&icenet_lock);
  nic_send((void *)pkt_buf, pkt_len);
  arch_spin_unlock(&icenet_lock);
  // recycle the pkt_buf
  bool res = fifo_push(&global_pkt_ff, pkt_buf, 0);
  if (!res) {
    printf("sendto_single global fifo_push error\n");
    exit(0);
  }
  return 1;
}
#endif