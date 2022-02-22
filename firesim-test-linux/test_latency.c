#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "encoding.h"
#include "mmio_linux.h"
#include "nic_linux.h"

// An ICMP packet is 44 bytes
#define PACKET_BYTES 44
#undef PACKET_WORDS
#define PACKET_WORDS 8

uint64_t out_packet[PACKET_WORDS];
uint64_t in_packet[PACKET_WORDS];

int main(void) {
  pin_process_memory();
  // setup MMIO mapping
  mmio_map(&__mmio, NIC_BASE, 12);

  uint64_t srcmac = nic_macaddr(), start, end;

  // NET_IP_ALIGN = 2
  out_packet[0] = srcmac << 16;
  out_packet[1] = srcmac | (0x0208L << 48);
  // *(uint64_t*)(((uint8_t*)out_packet + 6)) = srcmac << 16;
  // *(uint16_t*)(((uint8_t*)out_packet + 12)) = 0x0208;

  nic_post_recv((uint64_t)in_packet);

  start = rdcycle();
  nic_post_send((uint64_t)out_packet, PACKET_BYTES);

  while (((nic_counts() >> NIC_COUNT_RECV_COMP) & 0xf) == 0)
    ;
  nic_complete_recv();

  end = rdcycle();

  while (((nic_counts() >> NIC_COUNT_SEND_COMP) & 0xf) == 0)
    ;
  nic_complete_send();

  printf("Total send/recv time %lu\n", end - start);

  return 0;
}
