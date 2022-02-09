#ifndef RISCV_ICENIC_H
#define RISCV_ICENIC_H

#define SIMPLENIC_BASE 0x10016000L
#define SIMPLENIC_SEND_REQ (SIMPLENIC_BASE + 0)
#define SIMPLENIC_RECV_REQ (SIMPLENIC_BASE + 8)
#define SIMPLENIC_SEND_COMP (SIMPLENIC_BASE + 16)
#define SIMPLENIC_RECV_COMP (SIMPLENIC_BASE + 18)
#define SIMPLENIC_COUNTS (SIMPLENIC_BASE + 20)
#define SIMPLENIC_MACADDR (SIMPLENIC_BASE + 24)

#define NIC_COUNT_SEND_REQ 0
#define NIC_COUNT_RECV_REQ 8
#define NIC_COUNT_SEND_COMP 16
#define NIC_COUNT_RECV_COMP 24

#include "mmio.h"

static inline int nic_send_req_avail(void)
{
	return reg_read32(SIMPLENIC_COUNTS) & 0xff;
}

static inline int nic_recv_req_avail(void)
{
	return (reg_read32(SIMPLENIC_COUNTS) >> 8) & 0xff;
}

static inline int nic_send_comp_avail(void)
{
	return (reg_read32(SIMPLENIC_COUNTS) >> 16) & 0xff;
}

static inline int nic_recv_comp_avail(void)
{
	return (reg_read32(SIMPLENIC_COUNTS) >> 24) & 0xff;
}

static void nic_send(void *data, unsigned long len)
{
	uintptr_t addr = ((uintptr_t) data) & ((1L << 48) - 1);
	unsigned long packet = (len << 48) | addr;

	while (nic_send_req_avail() == 0);
	reg_write64(SIMPLENIC_SEND_REQ, packet);

	while (nic_send_comp_avail() == 0);
	reg_read16(SIMPLENIC_SEND_COMP);
}

static int nic_recv(void *dest)
{
	uintptr_t addr = (uintptr_t) dest;
	int len;

	while (nic_recv_req_avail() == 0);
	reg_write64(SIMPLENIC_RECV_REQ, addr);

	// Poll for completion
	while (nic_recv_comp_avail() == 0);
	len = reg_read16(SIMPLENIC_RECV_COMP);
	asm volatile ("fence");

	return len;
}

static inline uint32_t nic_counts(void)
{
	return reg_read32(SIMPLENIC_COUNTS);
}

static inline void nic_post_send_no_check(uint64_t addr, uint64_t len)
{
	uint64_t request = ((len & 0x7fff) << 48) | (addr & 0xffffffffffffL);
	reg_write64(SIMPLENIC_SEND_REQ, request);
}

static inline void nic_complete_send(void)
{
	reg_read16(SIMPLENIC_SEND_COMP);
}

static inline void nic_post_recv_no_check(uint64_t addr)
{
	reg_write64(SIMPLENIC_RECV_REQ, addr);    
}

static inline uint16_t nic_complete_recv(void)
{
	return reg_read16(SIMPLENIC_RECV_COMP);
}

static inline uint64_t nic_macaddr(void)
{
	return reg_read64(SIMPLENIC_MACADDR);
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

#define ceil_div(n, d) (((n) - 1) / (d) + 1)

// R type: .insn r opcode, func3, func7, rd, rs1, rs2

static inline uint16_t ntohs(uint16_t nint)
{
	uint16_t result;
	asm volatile (".insn r 0x33, 0, 0x2A, %0, %1, x0" : "=r"(result) : "r"(nint));
        return result;
}

static inline uint32_t ntohi(uint32_t nint)
{
	uint32_t result;
	asm volatile (".insn r 0x33, 1, 0x2A, %0, %1, x0" : "=r"(result) : "r"(nint));
        return result;
}

static inline uint64_t ntohl(uint64_t nint)
{
	uint64_t result;
	asm volatile (".insn r 0x33, 2, 0x2A, %0, %1, x0" : "=r"(result) : "r"(nint));
        return result;
}

static inline uint16_t htons(uint16_t nint)
{
        return ntohs(nint);
}

static inline uint32_t htoni(uint32_t nint)
{
	return ntohi(nint);
}

static inline uint64_t htonl(uint64_t nint)
{
	return ntohl(nint);
}

#define NET_IP_ALIGN 2
#define ETH_MAX_WORDS 258
#define ETH_HEADER_SIZE 14
#define MAC_ADDR_SIZE 6
#define IP_ADDR_SIZE 4

#define LNIC_HEADER_SIZE 30

struct eth_header {
        uint8_t pad[NET_IP_ALIGN];
        uint8_t dst_mac[MAC_ADDR_SIZE];
        uint8_t src_mac[MAC_ADDR_SIZE];
        uint16_t ethtype;
};

struct arp_header {
        uint16_t htype;
        uint16_t ptype;
        uint8_t hlen;
        uint8_t plen;
        uint16_t oper;
        uint8_t sha[MAC_ADDR_SIZE];
        uint8_t spa[IP_ADDR_SIZE];
        uint8_t tha[MAC_ADDR_SIZE];
        uint8_t tpa[IP_ADDR_SIZE];
};

struct ipv4_header {
        uint8_t ver_ihl;
        uint8_t dscp_ecn;
        uint16_t length;
        uint16_t ident;
        uint16_t flags_frag_off;
        uint8_t ttl;
        uint8_t proto;
        uint16_t cksum;
        uint32_t src_addr;
        uint32_t dst_addr;
};

struct icmp_header {
        uint8_t type;
        uint8_t code;
        uint16_t cksum;
        uint32_t rest;
};

struct lnic_header {
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

static int checksum(uint16_t *data, int len)
{
        int i;
        uint32_t sum = 0;

        for (i = 0; i < len; i++)
                sum += ntohs(data[i]);

        while ((sum >> 16) != 0)
                sum = (sum & 0xffff) + (sum >> 16);

        sum = ~sum & 0xffff;

        return sum;
}

/* Post a send request
 */
static void nic_post_send(void *buf, uint64_t len)
{
  uintptr_t addr = ((uintptr_t) buf) & ((1L << 48) - 1);
  unsigned long packet = (len << 48) | addr;
  while (nic_send_req_avail() == 0);
  reg_write64(SIMPLENIC_SEND_REQ, packet);
}

/* Wait for all send operations to complete
 */
static void nic_wait_send_batch(int tx_count)
{
  int i;

  for (i=0; i < tx_count; i++) {
    // Poll for completion
    while (nic_send_comp_avail() == 0);
    reg_read16(SIMPLENIC_SEND_COMP);
  }
}

/* Post a batch of RX requests
 */
static void nic_post_recv_batch(uint64_t bufs[][ETH_MAX_WORDS], int rx_count)
{
  int i;
  for (i=0; i < rx_count; i++) {
    uintptr_t addr = (uintptr_t) bufs[i];
    while (nic_recv_req_avail() == 0);
    reg_write64(SIMPLENIC_RECV_REQ, addr);
  }
}

/* Wait for a receive to complete
 */
static int nic_wait_recv()
{
  int len;

  // Poll for completion
  while (nic_recv_comp_avail() == 0);
  len = reg_read16(SIMPLENIC_RECV_COMP);
  asm volatile ("fence");

  return len;
}

/**
 * Receive and parse Eth/IP/LNIC headers.
 * Only return once lnic pkt is received.
 */
static int nic_recv_lnic(void *buf, struct lnic_header **lnic)
{
  struct ipv4_header *ipv4;
  int len;

  // receive pkt
  len = nic_recv(buf);

  ipv4 = buf + ETH_HEADER_SIZE;
  // parse lnic hdr
  int ihl = ipv4->ver_ihl & 0xf;
  *lnic = (void *)ipv4 + (ihl << 2);
  return len;
}

/**
 * Swap Ethernet addresses
 */
static int swap_eth(void *buf)
{
  struct eth_header *eth;
  uint8_t tmp_mac[MAC_ADDR_SIZE];

  eth = buf;
  // swap addresses
  memcpy(tmp_mac, eth->dst_mac, MAC_ADDR_SIZE);
  memcpy(eth->dst_mac, eth->src_mac, MAC_ADDR_SIZE);
  memcpy(eth->src_mac, tmp_mac, MAC_ADDR_SIZE);

  return 0;
}

/**
 * Swap addresses in lnic pkt
 */
static int swap_addresses(void *buf)
{
  struct eth_header *eth;
  struct ipv4_header *ipv4;
  struct lnic_header *lnic;
  uint32_t tmp_ip_addr;
  uint16_t tmp_lnic_addr;

  eth = buf;
  ipv4 = buf + ETH_HEADER_SIZE;
  int ihl = ipv4->ver_ihl & 0xf;
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

/**
 * Send one LNIC pkt to indicate that the system has booted
 * and is ready to start processing pkts.
 * Some Python unit tests wait for this boot pkt to arrive
 * before starting the tests.
 */
#define BOOT_PKT_LEN 72
static void nic_boot_pkt(void) {
  unsigned long len = BOOT_PKT_LEN;
  uint8_t buf[BOOT_PKT_LEN];

  struct eth_header *eth;
  struct ipv4_header *ipv4;
  struct lnic_header *lnic;

  uint64_t macaddr_long;
  uint8_t *macaddr;
  
  macaddr_long = ntohl(nic_macaddr());
  macaddr = ((uint8_t *)&macaddr_long) + 2;

  eth = (void *)buf;
  ipv4 = (void *)eth + ETH_HEADER_SIZE;
  lnic = (void *)ipv4 + 20;

  // Fill out header fields
  memset(&(eth->dst_mac), 0, MAC_ADDR_SIZE);
  memcpy(&(eth->src_mac), macaddr, MAC_ADDR_SIZE);
  eth->ethtype = htons(IPV4_ETHTYPE);

  ipv4->ver_ihl = 0x45;
  ipv4->dscp_ecn = 0;
  ipv4->length = htons(BOOT_PKT_LEN - ETH_HEADER_SIZE);
  ipv4->ident = htons(1);
  ipv4->flags_frag_off = htons(0);
  ipv4->ttl = 64;
  ipv4->proto = LNIC_PROTO;
  ipv4->cksum = 0; // NOTE: not implemented
  ipv4->src_addr = htoni(0);
  ipv4->dst_addr = htoni(0);

  lnic->flags = 1; // DATA pkt
  lnic->src = htons(0);
  lnic->dst = htons(0);
  lnic->msg_len = htons(BOOT_PKT_LEN - 64);
  lnic->pkt_offset = 0;
  lnic->pull_offset = 0;
  lnic->msg_id = 0;
  lnic->buf_ptr = 0;
  lnic->buf_size_class = 0;

  nic_send(buf, len);
}

#endif // RISCV_ICENIC_H
