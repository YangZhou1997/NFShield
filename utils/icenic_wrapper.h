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
  nic_send(nf_idx, buf, len);
  arch_spin_unlock(&icenet_lock);
}

int recvfrom_single(int core_id, intptr_t pkt_buf, int *pkt_len) {
  arch_spin_lock(&icenet_lock);
  *pkt_len = nic_recv(core_id, (void *)pkt_buf);
  arch_spin_unlock(&icenet_lock);
  return 1;
}

int sendto_single(int core_id, intptr_t pkt_buf, int pkt_len) {
  arch_spin_lock(&icenet_lock);
  nic_send(core_id, (void *)pkt_buf, pkt_len);
  arch_spin_unlock(&icenet_lock);
  return 1;
}
#endif