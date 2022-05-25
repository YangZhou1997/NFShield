#ifndef RISCV_ICENIC_H
#define RISCV_ICENIC_H

// this file is mostly adapted from NanoPU:
// https://github.com/l-nic/chipyard/blob/lnic-dev/tests-icenic/icenic.h, which
// implements simple post_send/recv(), and check_send/recv() functionality
// around the icenic registers

#define SIMPLENIC_BASE 0x10016000L
#define STEP_SIZE (0x40)
const int SIMPLENIC_SEND_REQ[4] = {
    (SIMPLENIC_BASE + 0), (SIMPLENIC_BASE + 0 + STEP_SIZE),
    (SIMPLENIC_BASE + 0 + STEP_SIZE * 2), (SIMPLENIC_BASE + 0 + STEP_SIZE * 3)};
const int SIMPLENIC_RECV_REQ[4] = {
    (SIMPLENIC_BASE + 8), (SIMPLENIC_BASE + 8 + STEP_SIZE),
    (SIMPLENIC_BASE + 8 + STEP_SIZE * 2), (SIMPLENIC_BASE + 8 + STEP_SIZE * 3)};
const int SIMPLENIC_SEND_COMP[4] = {(SIMPLENIC_BASE + 16),
                                    (SIMPLENIC_BASE + 16 + STEP_SIZE),
                                    (SIMPLENIC_BASE + 16 + STEP_SIZE * 2),
                                    (SIMPLENIC_BASE + 16 + STEP_SIZE * 3)};
const int SIMPLENIC_RECV_COMP[4] = {(SIMPLENIC_BASE + 18),
                                    (SIMPLENIC_BASE + 18 + STEP_SIZE),
                                    (SIMPLENIC_BASE + 18 + STEP_SIZE * 2),
                                    (SIMPLENIC_BASE + 18 + STEP_SIZE * 3)};
const int SIMPLENIC_COUNTS[4] = {(SIMPLENIC_BASE + 20),
                                 (SIMPLENIC_BASE + 20 + STEP_SIZE),
                                 (SIMPLENIC_BASE + 20 + STEP_SIZE * 2),
                                 (SIMPLENIC_BASE + 20 + STEP_SIZE * 3)};
const int SIMPLENIC_MACADDR[4] = {(SIMPLENIC_BASE + 24),
                                  (SIMPLENIC_BASE + 24 + STEP_SIZE),
                                  (SIMPLENIC_BASE + 24 + STEP_SIZE * 2),
                                  (SIMPLENIC_BASE + 24 + STEP_SIZE * 3)};

#define NIC_COUNT_SEND_REQ 0
#define NIC_COUNT_RECV_REQ 8
#define NIC_COUNT_SEND_COMP 16
#define NIC_COUNT_RECV_COMP 24

#include "common.h"
#include "mmio.h"
#include "pkt-header.h"

static inline int nic_send_req_avail(int cid) {
  return reg_read32(SIMPLENIC_COUNTS[cid]) & 0xff;
}

static inline int nic_recv_req_avail(int cid) {
  return (reg_read32(SIMPLENIC_COUNTS[cid]) >> 8) & 0xff;
}

static inline int nic_send_comp_avail(int cid) {
  return (reg_read32(SIMPLENIC_COUNTS[cid]) >> 16) & 0xff;
}

static inline int nic_recv_comp_avail(int cid) {
  return (reg_read32(SIMPLENIC_COUNTS[cid]) >> 24) & 0xff;
}

static void nic_send(int cid, void *data, unsigned long len) {
  uintptr_t addr = ((uintptr_t)data) & ((1L << 48) - 1);
  unsigned long packet = (len << 48) | addr;

  while (nic_send_req_avail(cid) == 0)
    ;
  reg_write64(SIMPLENIC_SEND_REQ[cid], packet);

  while (nic_send_comp_avail(cid) == 0)
    ;
  reg_read16(SIMPLENIC_SEND_COMP[cid]);
}

static int nic_recv(int cid, void *dest) {
  uintptr_t addr = (uintptr_t)dest;
  int len;

  while (nic_recv_req_avail(cid) == 0)
    ;
  reg_write64(SIMPLENIC_RECV_REQ[cid], addr);

  // Poll for completion
  while (nic_recv_comp_avail(cid) == 0)
    ;
  len = reg_read16(SIMPLENIC_RECV_COMP[cid]);
  asm volatile("fence");

  return len;
}

static inline uint32_t nic_counts(int cid) {
  return reg_read32(SIMPLENIC_COUNTS[cid]);
}

static inline void nic_post_send_no_check(int cid, uint64_t addr,
                                          uint64_t len) {
  uint64_t request = ((len & 0x7fff) << 48) | (addr & 0xffffffffffffL);
  reg_write64(SIMPLENIC_SEND_REQ[cid], request);
}

static inline void nic_complete_send(int cid) {
  reg_read16(SIMPLENIC_SEND_COMP[cid]);
}

static inline void nic_post_recv_no_check(int cid, uint64_t addr) {
  reg_write64(SIMPLENIC_RECV_REQ[cid], addr);
}

static inline uint16_t nic_complete_recv(int cid) {
  return reg_read16(SIMPLENIC_RECV_COMP[cid]);
}

static inline uint64_t nic_macaddr(int cid) {
  return reg_read64(SIMPLENIC_MACADDR[cid]);
}

#define IPV4_ETHTYPE 0x0800
#define ARP_ETHTYPE 0x0806
#define ICMP_PROTO 1
#define LNIC_PROTO 0x99
#define ECHO_REPLY 0
#define ECHO_REQUEST 8
#define ARP_REQUEST 1
#define ARP_REPLY 2
#define HTYPE_ETH 1

#define ceil_div(n, d) (((n)-1) / (d) + 1)

// R type: .insn r opcode, func3, func7, rd, rs1, rs2

static inline uint16_t ntohs(uint16_t nint) {
  // uint16_t result;
  // asm volatile(".insn r 0x33, 0, 0x2A, %0, %1, x0" : "=r"(result) :
  // "r"(nint)); return result;
  return ((nint & 0xff) << 8) | ((nint >> 8) & 0xff);
}

static inline uint32_t ntohi(uint32_t nint) {
  // uint32_t result;
  // asm volatile(".insn r 0x33, 1, 0x2A, %0, %1, x0" : "=r"(result) :
  // "r"(nint)); return result;
  return ((nint & 0xff) << 24) | ((nint & 0xff00) << 8) |
         ((nint & 0xff0000) >> 8) | ((nint >> 24) & 0xff);
}

static inline uint64_t ntohl(uint64_t nint) {
  // uint64_t result;
  // asm volatile(".insn r 0x33, 2, 0x2A, %0, %1, x0" : "=r"(result) :
  // "r"(nint)); return result;
  uint64_t rval;
  uint8_t *data = (uint8_t *)&rval;

  data[0] = nint >> 56;
  data[1] = nint >> 48;
  data[2] = nint >> 40;
  data[3] = nint >> 32;
  data[4] = nint >> 24;
  data[5] = nint >> 16;
  data[6] = nint >> 8;
  data[7] = nint >> 0;

  return rval;
}

static inline uint16_t htons(uint16_t nint) { return ntohs(nint); }

static inline uint32_t htoni(uint32_t nint) { return ntohi(nint); }

static inline uint64_t htonl(uint64_t nint) { return ntohl(nint); }

#define LNIC_HEADER_SIZE 30

struct lnic_hdr {
  uint8_t flags;
  uint16_t src;
  uint16_t dst;
  uint16_t msg_len;
  uint8_t pkt_offset;
  uint16_t pull_offset;
  uint16_t msg_id;
  uint16_t buf_ptr;
  uint8_t buf_size_class;
  uint64_t pad0;
  uint32_t pad1;
  uint16_t pad2;
  uint8_t pad3;
} __attribute__((packed));

static int checksum(uint16_t *data, int len) {
  int i;
  uint32_t sum = 0;

  for (i = 0; i < len; i++) sum += ntohs(data[i]);

  while ((sum >> 16) != 0) sum = (sum & 0xffff) + (sum >> 16);

  sum = ~sum & 0xffff;

  return sum;
}

/* Post a send request
 */
static void nic_post_send(int cid, void *buf, uint64_t len) {
  uintptr_t addr = ((uintptr_t)buf) & ((1L << 48) - 1);
  unsigned long packet = (len << 48) | addr;
  while (nic_send_req_avail(cid) == 0)
    ;
  reg_write64(SIMPLENIC_SEND_REQ[cid], packet);
}

/* Wait for all send operations to complete
 */
static void nic_wait_send_batch(int cid, int tx_count) {
  int i;

  for (i = 0; i < tx_count; i++) {
    // Poll for completion
    while (nic_send_comp_avail(cid) == 0)
      ;
    reg_read16(SIMPLENIC_SEND_COMP[cid]);
  }
}

/* Post a batch of RX requests
 */
static void nic_post_recv_batch(int cid, uint64_t bufs[][ETH_MAX_WORDS],
                                int rx_count) {
  int i;
  for (i = 0; i < rx_count; i++) {
    uintptr_t addr = (uintptr_t)bufs[i];
    while (nic_recv_req_avail(cid) == 0)
      ;
    reg_write64(SIMPLENIC_RECV_REQ[cid], addr);
  }
}

/* Wait for a receive to complete
 */
static int nic_wait_recv(int cid) {
  int len;

  // Poll for completion
  while (nic_recv_comp_avail(cid) == 0)
    ;
  len = reg_read16(SIMPLENIC_RECV_COMP[cid]);
  asm volatile("fence");

  return len;
}

/**
 * Receive and parse Eth/IP/LNIC headers.
 * Only return once lnic pkt is received.
 */
static int nic_recv_lnic(int cid, void *buf, struct lnic_hdr **lnic) {
  struct ipv4_hdr *ipv4;
  int len;

  // receive pkt
  len = nic_recv(cid, buf);

  ipv4 = buf + ETH_HEADER_SIZE;
  // parse lnic hdr
  int ihl = ipv4->version_ihl & 0xf;
  *lnic = (void *)ipv4 + (ihl << 2);
  return len;
}

/**
 * Swap Ethernet addresses
 */
static int swap_eth(void *buf) {
  struct ether_hdr *eth;
  uint8_t tmp_mac[MAC_ADDR_SIZE];

  eth = buf;
  // swap addresses
  memcpy(tmp_mac, eth->d_addr.addr_bytes, MAC_ADDR_SIZE);
  memcpy(eth->d_addr.addr_bytes, eth->s_addr.addr_bytes, MAC_ADDR_SIZE);
  memcpy(eth->s_addr.addr_bytes, tmp_mac, MAC_ADDR_SIZE);

  return 0;
}

/**
 * Swap addresses in lnic pkt
 */
static int swap_addresses(void *buf) {
  struct ether_hdr *eth;
  struct ipv4_hdr *ipv4;
  struct lnic_hdr *lnic;
  uint32_t tmp_ip_addr;
  uint16_t tmp_lnic_addr;

  eth = buf;
  ipv4 = buf + ETH_HEADER_SIZE;
  int ihl = ipv4->version_ihl & 0xf;
  lnic = (void *)ipv4 + (ihl << 2);

  // swap eth/ip/lnic src and dst
  swap_eth(eth);

  tmp_ip_addr = ipv4->dst_addr;
  ipv4->dst_addr = ipv4->src_addr;
  ipv4->src_addr = tmp_ip_addr;

  tmp_lnic_addr = lnic->dst;
  lnic->dst = lnic->src;
  lnic->src = tmp_lnic_addr;

  return 0;
}

#endif  // RISCV_ICENIC_H
