#ifndef __PKT_HDR_OPS_H__
#define __PKT_HDR_OPS_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "pkt-header.h"
#include "fnv64.h"

static inline void swap_mac_addr(uint8_t *pkt_ptr) {
  struct ether_hdr *eth;
  uint8_t tmp_mac[MAC_ADDR_SIZE];

  eth = (struct ether_hdr *)pkt_ptr;
  // swap addresses
  memcpy(tmp_mac, eth->d_addr.addr_bytes, MAC_ADDR_SIZE);
  memcpy(eth->d_addr.addr_bytes, eth->s_addr.addr_bytes, MAC_ADDR_SIZE);
  memcpy(eth->s_addr.addr_bytes, tmp_mac, MAC_ADDR_SIZE);
}

#define REVERSE(key)                                                \
  {                                                                 \
    .srcip = key.dstip, .dstip = key.srcip, .srcport = key.dstport, \
    .dstport = key.srcport, .proto = key.proto                      \
  }
  
uint16_t get_eth_type(uint8_t *pkt_ptr) {
  struct ether_hdr *eth = (struct ether_hdr *)pkt_ptr;
  return eth->ether_type;
}
void set_eth_type(uint8_t *pkt_ptr, uint16_t eth_type) {
  struct ether_hdr *eth = (struct ether_hdr *)pkt_ptr;
  eth->ether_type = eth_type;
}
uint64_t get_src_mac(uint8_t *pkt_ptr) {
  struct ether_hdr *eth = (struct ether_hdr *)pkt_ptr;
  uint64_t mac = 0;
  memcpy(&mac, eth->s_addr.addr_bytes, MAC_ADDR_SIZE);
  return mac;
}
void set_src_mac(uint8_t *pkt_ptr, uint64_t mac_addr) {
  struct ether_hdr *eth = (struct ether_hdr *)pkt_ptr;
  memcpy(eth->s_addr.addr_bytes, &mac_addr, MAC_ADDR_SIZE);
}
uint64_t get_dst_mac(uint8_t *pkt_ptr) {
  struct ether_hdr *eth = (struct ether_hdr *)pkt_ptr;
  uint64_t mac = 0;
  memcpy(&mac, eth->d_addr.addr_bytes, MAC_ADDR_SIZE);
  return mac;
}
void set_dst_mac(uint8_t *pkt_ptr, uint64_t mac_addr) {
  struct ether_hdr *eth = (struct ether_hdr *)pkt_ptr;
  memcpy(eth->d_addr.addr_bytes, &mac_addr, MAC_ADDR_SIZE);
}

uint32_t get_src_ip(uint8_t *pkt_ptr) {
  struct ipv4_hdr *ip = (struct ipv4_hdr *)(pkt_ptr + ETH_HEADER_SIZE);
  return ip->src_addr;
}
void set_src_ip(uint8_t *pkt_ptr, uint32_t ip_addr) {
  struct ipv4_hdr *ip = (struct ipv4_hdr *)(pkt_ptr + ETH_HEADER_SIZE);
  ip->src_addr = ip_addr;
}
uint32_t get_dst_ip(uint8_t *pkt_ptr) {
  struct ipv4_hdr *ip = (struct ipv4_hdr *)(pkt_ptr + ETH_HEADER_SIZE);
  return ip->dst_addr;
}
void set_dst_ip(uint8_t *pkt_ptr, uint32_t ip_addr) {
  struct ipv4_hdr *ip = (struct ipv4_hdr *)(pkt_ptr + ETH_HEADER_SIZE);
  ip->dst_addr = ip_addr;
}

uint16_t get_src_port(uint8_t *pkt_ptr) {
  struct tcp_hdr *tcp = (struct tcp_hdr *)(pkt_ptr + ETH_HEADER_SIZE + IP_HEADER_SIZE);
  return tcp->src_port;
}
void set_src_port(uint8_t *pkt_ptr, uint16_t port_num) {
  struct tcp_hdr *tcp = (struct tcp_hdr *)(pkt_ptr + ETH_HEADER_SIZE + IP_HEADER_SIZE);
  tcp->src_port = port_num;
}
uint16_t get_dst_port(uint8_t *pkt_ptr) {
  struct tcp_hdr *tcp = (struct tcp_hdr *)(pkt_ptr + ETH_HEADER_SIZE + IP_HEADER_SIZE);
  return tcp->dst_port;
}
void set_dst_port(uint8_t *pkt_ptr, uint16_t port_num) {
  struct tcp_hdr *tcp = (struct tcp_hdr *)(pkt_ptr + ETH_HEADER_SIZE + IP_HEADER_SIZE);
  tcp->dst_port = port_num;
}

uint8_t get_proto(uint8_t *pkt_ptr) {
  struct ipv4_hdr *ip = (struct ipv4_hdr *)(pkt_ptr + ETH_HEADER_SIZE);
  return ip->next_proto_id;
}
void set_proto(uint8_t *pkt_ptr, uint8_t proto) {
  struct ipv4_hdr *ip = (struct ipv4_hdr *)(pkt_ptr + ETH_HEADER_SIZE);
  ip->next_proto_id = proto;
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
