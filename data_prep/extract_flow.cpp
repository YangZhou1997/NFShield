#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <unordered_map>

#include "common.h"
#include "fnv64.h"

using namespace std;

struct hashfn {
  size_t operator()(five_tuple_t const &key) const {
    uint32_t value;
    value = (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
    return value;
  }
};
struct equalfn {
  bool operator()(five_tuple_t const &t1, five_tuple_t const &t2) const {
    return IS_EQUAL(t1, t2);
  }
};
struct ether_hdr stand_eth_hdr;

unordered_map<five_tuple_t, bool, hashfn, equalfn> mp;

#define MAX_PAYLOAD_SIZE (1024 + 512)
#define PKT_HEADER_SIZE \
  (sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct tcp_hdr))

static int read_mac(struct ether_addr *mac, FILE *file) {
  memset(mac, 0, sizeof(struct ether_addr));
  int i = 0, start = 0;
  while (1) {
    char c = fgetc(file);
    if (c == ':') {
      i++;
    } else if (c <= '9' && c >= '0') {
      mac->addr_bytes[i] = mac->addr_bytes[i] * 16 + (c - '0');
      start = 1;
    } else if (c <= 'f' && c >= 'a') {
      mac->addr_bytes[i] = mac->addr_bytes[i] * 16 + (c - 'a' + 10);
      start = 1;
    } else if (start == 1)
      break;
  }
#if 0
	for (i = 0; i < 6; i++)
		printf("%hhx ", mac->addr_bytes[i]);
	printf("\n");
#endif
  return 0;
}

struct Config {
  struct ether_addr srcmac, dstmac;
  uint32_t srcip;
  uint32_t srcip_start, srcip_end, srcip_inc;
  uint32_t dstip;
  uint16_t sport;
  uint16_t sport_start, sport_end, sport_inc;
  double rate;
  int burst_size;
  uint32_t pkt_length;
  uint32_t trace_time;
  int new_flow_rate;  // new_flow_rate/10000
  int reverse_pct;    // percentage of out2in flow
};
struct Config config;

five_tuple_t distinct_5tuple[1 * 1024 * 1024 + 10];

static void build_five_tuple(char *file_path, uint64_t npkt) {
  printf("trace path: %s\n", file_path);
  five_tuple_t temp;

  FILE *file = fopen(file_path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file %s\n", file_path);
    exit(0);
  }

  char iph[MAX_PAYLOAD_SIZE];
  printf("building reference packets ...\n");
  fflush(stdout);
  uint32_t pkt_build_count = 0;

  int distinct_pkt_num = 0;

  while (pkt_build_count < npkt) {
    if (pkt_build_count % (1024 * 1024) == 0) {
      printf("%d\n", pkt_build_count);
      fflush(stdout);
    }

    fread(iph, 1, sizeof(struct ipv4_hdr), file);
    uint16_t pkt_len = htons(((struct ipv4_hdr *)iph)->total_length);
    fread(iph + sizeof(struct ipv4_hdr), 1, pkt_len - sizeof(struct ipv4_hdr),
          file);

    struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *)(iph);
    struct tcp_hdr *tcp_hdr = (struct tcp_hdr *)(iph + 20);

    uint16_t ipv4_hdr_len = (((struct ipv4_hdr *)iph)->version_ihl & 0x0F) << 2;
    uint16_t tcp_hdr_len =
        ((((struct tcp_hdr *)(iph + 20))->data_off & 0xF0) >> 4) << 2;

    if (ipv4_hdr_len != 20) {
      printf("ipv4_hdr_len errors: %hu\n", ipv4_hdr_len);
      fflush(stdout);
    }
    if (tcp_hdr_len != 20) {
      printf("tcp_hdr_len errors: %hu\n", tcp_hdr_len);
      fflush(stdout);
    }
    char *pkt_content = (char *)malloc(pkt_len + sizeof(struct ether_hdr) + 1);
    if (pkt_content == NULL) {
      printf("malloc errors!\n");
      fflush(stdout);
      exit(0);
    }
    memcpy(pkt_content, &stand_eth_hdr, sizeof(struct ether_hdr));
    memcpy(pkt_content + sizeof(struct ether_hdr), iph, pkt_len);

    uint16_t eth_pkt_len = pkt_len + sizeof(struct ether_hdr);

    temp.srcip = ip_hdr->src_addr;
    temp.dstip = ip_hdr->dst_addr;
    temp.srcport = tcp_hdr->src_port;
    temp.dstport = tcp_hdr->dst_port;
    temp.proto = ip_hdr->next_proto_id;

    if (mp.find(temp) == mp.end()) {
      distinct_5tuple[distinct_pkt_num++] = temp;
    }
    mp[temp]++;

    free(pkt_content);
    if (mp.size() == 100 * 1024) {
      return;
    }
    pkt_build_count++;
  }
  fclose(file);
}

static void build_pkt_content(char *file_path, uint64_t npkt) {
  FILE *file = fopen("./common/l2.conf", "r");
  char opt[100];
  while (fscanf(file, "%s", opt) != EOF) {
    if (strcmp(opt, "src.mac") == 0) {
      read_mac(&config.srcmac, file);
      stand_eth_hdr.s_addr = config.srcmac;
    } else if (strcmp(opt, "dst.mac") == 0) {
      read_mac(&config.dstmac, file);
      stand_eth_hdr.d_addr = config.dstmac;
      stand_eth_hdr.ether_type = htons(0x800);
    }
  }
  fclose(file);

  printf("trace path: %s\n", file_path);
  five_tuple_t temp;

  file = fopen(file_path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file %s\n", file_path);
    exit(0);
  }

  char iph[MAX_PAYLOAD_SIZE];
  printf("building reference packets ...\n");
  fflush(stdout);
  uint32_t pkt_build_count = 0;

  FILE *f_gem5 = fopen("../ictf2010_100kflow.dat", "w");
  uint64_t pkt_size = 0;
  int distinct_pkt_num = 0;
  int pkt_passing = 0;

  while (pkt_passing < npkt) {
    if (pkt_passing % (1024 * 1024) == 0) {
      printf("%d\n", pkt_passing);
      fflush(stdout);
    }
    pkt_passing++;

    fread(iph, 1, sizeof(struct ipv4_hdr), file);
    uint16_t pkt_len = htons(((struct ipv4_hdr *)iph)->total_length);
    fread(iph + sizeof(struct ipv4_hdr), 1, pkt_len - sizeof(struct ipv4_hdr),
          file);

    struct ipv4_hdr *ip_hdr = (struct ipv4_hdr *)(iph);
    struct tcp_hdr *tcp_hdr = (struct tcp_hdr *)(iph + 20);

    temp = distinct_5tuple[pkt_build_count];
    ip_hdr->src_addr = temp.srcip;
    ip_hdr->dst_addr = temp.dstip;
    tcp_hdr->src_port = temp.srcport;
    tcp_hdr->dst_port = temp.dstport;
    ip_hdr->next_proto_id = temp.proto;

    uint16_t ipv4_hdr_len = (((struct ipv4_hdr *)iph)->version_ihl & 0x0F) << 2;
    uint16_t tcp_hdr_len =
        ((((struct tcp_hdr *)(iph + 20))->data_off & 0xF0) >> 4) << 2;

    if (ipv4_hdr_len != 20) {
      printf("ipv4_hdr_len errors: %hu\n", ipv4_hdr_len);
      fflush(stdout);
    }
    if (tcp_hdr_len != 20) {
      printf("tcp_hdr_len errors: %hu\n", tcp_hdr_len);
      fflush(stdout);
    }
    // Randomly sample 1M packet contents.
    if (rand() * 1.0 / RAND_MAX > 0.01) {
      continue;
    }

    char *pkt_content = (char *)malloc(pkt_len + sizeof(struct ether_hdr) + 1);
    if (pkt_content == NULL) {
      printf("malloc errors!\n");
      fflush(stdout);
      exit(0);
    }
    memcpy(pkt_content, &stand_eth_hdr, sizeof(struct ether_hdr));
    memcpy(pkt_content + sizeof(struct ether_hdr), iph, pkt_len);

    uint16_t eth_pkt_len = pkt_len + sizeof(struct ether_hdr);

    pkt_size += eth_pkt_len;
    fwrite(&eth_pkt_len, sizeof(uint16_t), 1, f_gem5);
    fwrite(pkt_content, 1, eth_pkt_len, f_gem5);

    free(pkt_content);
    if (pkt_build_count == 100 * 1024) {
      printf("average packet size = %lf\n", pkt_size * 1.0 / mp.size());
      break;
    }
    pkt_build_count++;
  }
  fclose(file);
  fclose(f_gem5);
}

int main() {
  srandom((unsigned)time(NULL));
  char *file_path = "../../normal_traffic/ictf2010.dat";
  build_five_tuple(file_path, 1000 * 1024 * 1024);
  printf("pkt num: %lu\n", mp.size());

  build_pkt_content(file_path, 1000 * 1024 * 1024);
  return 0;
}
