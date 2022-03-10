#ifndef __RAW_ICENET_
#define __RAW_ICENET_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "icenic_util.h"
#include "pkt-header.h"
#include "riscv_util.h"

arch_spinlock_t icenet_lock;

#ifndef NCORES
#define NCORES 4
#endif

/**
 * Send one LNIC pkt to indicate that the system has booted
 * and is ready to start processing pkts.
 * The switch wait for all boot pkts from all cores to arrive
 * before starting the tests.
 */
#define BOOT_PKT_LEN 64
static void nic_boot_pkt(int nf_idx) {
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
  ipv4->packet_id = htons(1);
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

#define NON_BLOCKINNG_RECV
int recvfrom_batch(int core_id, int buff_size, pkt_t *pkt_buf) {
  arch_spin_lock(&icenet_lock);
  int cnt = 0;
  while (cnt < MAX_BATCH_SIZE) {
#ifdef NON_BLOCKINNG_RECV
    if (nic_recv_req_avail() == 0) {
      arch_spin_unlock(&icenet_lock);
      return cnt;
    }
#else
    while (nic_recv_req_avail() == 0) ;
#endif
    nic_post_recv_no_check((uintptr_t)pkt_buf[cnt].content);
    pkt_buf[cnt].len = nic_wait_recv();
    cnt++;
  }
  arch_spin_unlock(&icenet_lock);
  return cnt;
}

int recvfrom_single(int core_id, int buff_size, pkt_t *pkt_buf) {
  arch_spin_lock(&icenet_lock);
#ifdef NON_BLOCKINNG_RECV
  if (nic_recv_req_avail() == 0) {
    arch_spin_unlock(&icenet_lock);
    return 0;
  }
#else
  while (nic_recv_req_avail() == 0);
#endif
  nic_post_recv_no_check((uintptr_t)pkt_buf->content);
  pkt_buf->len = nic_wait_recv();
  arch_spin_unlock(&icenet_lock);
  return 1;
}

int sendto_batch(int core_id, int batch_size, pkt_t *pkt_buf) {
  arch_spin_lock(&icenet_lock);
  int cnt = 0;
  while (cnt < batch_size) {
    nic_post_send(pkt_buf[cnt].content, pkt_buf[cnt].len);
    cnt++;
  }
  nic_wait_send_batch(cnt);
  arch_spin_unlock(&icenet_lock);
  return cnt;
}

int sendto_single(int core_id, pkt_t *pkt_buf) {
  arch_spin_lock(&icenet_lock);
  nic_send(pkt_buf->content, pkt_buf->len);
  arch_spin_unlock(&icenet_lock);
  return 1;
}
#endif