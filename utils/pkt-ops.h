#ifndef __PKT_HDR_OPS_H__
#define __PKT_HDR_OPS_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fnv64.h"

static inline void swap_mac_addr(uint8_t *pkt_ptr) {
  uint16_t s;
  uint32_t w;
  printf("a1: %d\n", (int)pkt_ptr[0]);
  printf("a2: %d\n", (int)pkt_ptr[1]);
  printf("a3: %d\n", (int)pkt_ptr[2]);
  printf("a4: %d\n", (int)pkt_ptr[3]);
  printf("a5: %d\n", (int)pkt_ptr[4]);
  printf("a6: %d\n", (int)pkt_ptr[5]);
  printf("a7: %d\n", (int)pkt_ptr[6]);
  printf("a8: %d\n", (int)pkt_ptr[7]);

  printf("swap_mac_addr a\n");
  /* assuming an IP/IPV6 pkt i.e. L2 header is 2 byte aligned, 4 byte
   * non-aligned */
  s = *(uint16_t *)pkt_ptr;
  printf("swap_mac_addr b %u\n", *(uint32_t *)(&pkt_ptr[2]));
  w = *(uint32_t *)(pkt_ptr + 2);
  printf("swap_mac_addr c\n");
  *(uint16_t *)pkt_ptr = *(uint16_t *)(pkt_ptr + 6);
  printf("swap_mac_addr d\n");
  *(uint32_t *)(pkt_ptr + 2) = *(uint32_t *)(pkt_ptr + 8);
  printf("swap_mac_addr e\n");
  *(uint16_t *)(pkt_ptr + 6) = s;
  printf("swap_mac_addr f\n");
  *(uint32_t *)(pkt_ptr + 8) = w;
  printf("swap_mac_addr g\n");
}

#define REVERSE(key)                                                \
  {                                                                 \
    .srcip = key.dstip, .dstip = key.srcip, .srcport = key.dstport, \
    .dstport = key.srcport, .proto = key.proto                      \
  }

// five_tuple_t reverse(five_tuple_t key)
// {
//     five_tuple_t new_key = { .srcip = key.dstip, .dstip = key.srcip, .srcport
//     = key.dstport, .dstport = key.srcport, .proto = key.proto}; return
//     new_key;
// }

uint16_t get_eth_type(uint8_t *pkt_ptr) {
  uint16_t eth_type = 0;
  eth_type = *CAST_PTR(uint16_t *, pkt_ptr + 12);
  return eth_type;
}
void set_eth_type(uint8_t *pkt_ptr, uint16_t eth_type) {
  *CAST_PTR(uint16_t *, pkt_ptr + 12) = eth_type;
}
uint64_t get_src_mac(uint8_t *pkt_ptr) {
  uint64_t mac = 0;
  mac |= CAST_PTR(uint64_t, (*CAST_PTR(uint16_t *, pkt_ptr + 6))) << 32;
  mac |= (*CAST_PTR(uint32_t *, pkt_ptr + 8));
  return mac;
}
void set_src_mac(uint8_t *pkt_ptr, uint64_t mac_addr) {
  *CAST_PTR(uint16_t *, pkt_ptr + 6) = CAST_PTR(uint16_t, mac_addr >> 32);
  *CAST_PTR(uint32_t *, pkt_ptr + 8) =
      CAST_PTR(uint32_t, mac_addr & 0xFFFFFFFF);
}
uint64_t get_dst_mac(uint8_t *pkt_ptr) {
  uint64_t mac = 0;
  mac |= CAST_PTR(uint64_t, (*CAST_PTR(uint16_t *, pkt_ptr))) << 32;
  mac |= (*CAST_PTR(uint32_t *, pkt_ptr + 2));
  return mac;
}
void set_dst_mac(uint8_t *pkt_ptr, uint64_t mac_addr) {
  *CAST_PTR(uint16_t *, pkt_ptr) = CAST_PTR(uint16_t, mac_addr >> 32);
  *CAST_PTR(uint32_t *, pkt_ptr + 2) =
      CAST_PTR(uint32_t, mac_addr & 0xFFFFFFFF);
}

uint32_t get_src_ip(uint8_t *pkt_ptr) {
  uint32_t ip = *CAST_PTR(uint32_t *, pkt_ptr + 14 + 12);
  return ip;
}
void set_src_ip(uint8_t *pkt_ptr, uint32_t ip_addr) {
  *CAST_PTR(uint32_t *, pkt_ptr + 14 + 12) = ip_addr;
}
uint32_t get_dst_ip(uint8_t *pkt_ptr) {
  uint32_t ip = *CAST_PTR(uint32_t *, pkt_ptr + 14 + 16);
  return ip;
}
void set_dst_ip(uint8_t *pkt_ptr, uint32_t ip_addr) {
  *CAST_PTR(uint32_t *, pkt_ptr + 14 + 16) = ip_addr;
}

uint16_t get_src_port(uint8_t *pkt_ptr) {
  // uint8_t ihl = *CAST_PTR(uint8_t *, pkt_ptr + 14) & 0xF; // ihl * 4 is the
  // IP header length.
  uint16_t port = *CAST_PTR(uint16_t *, pkt_ptr + 14 + 20);
  return port;
}
void set_src_port(uint8_t *pkt_ptr, uint16_t port_num) {
  // uint8_t ihl = *CAST_PTR(uint8_t *, pkt_ptr + 14) & 0xF; // ihl * 4 is the
  // IP header length.
  *CAST_PTR(uint16_t *, pkt_ptr + 14 + 20) = port_num;
}
uint16_t get_dst_port(uint8_t *pkt_ptr) {
  // uint8_t ihl = *CAST_PTR(uint8_t *, pkt_ptr + 14) & 0xF; // ihl * 4 is the
  // IP header length.
  uint16_t port = *CAST_PTR(uint16_t *, pkt_ptr + 14 + 20 + 2);
  return port;
}
void set_dst_port(uint8_t *pkt_ptr, uint16_t port_num) {
  // uint8_t ihl = *CAST_PTR(uint8_t *, pkt_ptr + 14) & 0xF; // ihl * 4 is the
  // IP header length.
  *CAST_PTR(uint16_t *, pkt_ptr + 14 + 20 + 2) = port_num;
}

uint8_t get_proto(uint8_t *pkt_ptr) {
  uint8_t proto = *CAST_PTR(uint8_t *, pkt_ptr + 14 + 9);
  return proto;
}
void set_proto(uint8_t *pkt_ptr, uint8_t proto) {
  *CAST_PTR(uint8_t *, pkt_ptr + 14 + 9) = CAST_PTR(uint8_t, proto);
}

void set_five_tuple(uint8_t *pkt_ptr, five_tuple_t key) {
  set_src_ip(pkt_ptr, key.srcip);
  set_dst_ip(pkt_ptr, key.dstip);
  set_src_port(pkt_ptr, key.srcport);
  set_dst_port(pkt_ptr, key.dstport);
  set_proto(pkt_ptr, key.proto);
}

void get_five_tuple(uint8_t *pkt_ptr, five_tuple_t *key) {
  key->srcip = get_src_ip(pkt_ptr);
  key->dstip = get_dst_ip(pkt_ptr);
  key->srcport = get_src_port(pkt_ptr);
  key->dstport = get_dst_port(pkt_ptr);
  key->proto = get_proto(pkt_ptr);
}

#endif /* __PKT_HDR_OPS_H__ */
