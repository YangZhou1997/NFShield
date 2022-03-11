#ifndef __PKT_PULLER_H__
#define __PKT_PULLER_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pkt-header.h"
#include "zipf-gen.h"

#define PKT_NUM (100 * 1024)

pkt_t pkts[PKT_NUM];

void load_pkt(char *filename) {
  if (strcmp(filename, "./data/ictf2010_100kflow.dat") != 0) {
    printf("pkt_puller3 must use trace of ./data/ictf2010_100kflow.dat\n");
    exit(0);
  }

  printf("trying to open file %s\n", filename);
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    printf("file open error\n");
    exit(0);
  }
  printf("opening succeeds\n");
  uint32_t pkt_cnt = 0;
  uint32_t pkt_size = 0;
  while (1) {
    fread(&pkts[pkt_cnt].len, sizeof(uint16_t), 1, file);
    pkt_size += pkts[pkt_cnt].len;
    fread(pkts[pkt_cnt].content, 1, pkts[pkt_cnt].len, file);
    pkt_cnt++;
    if (pkt_cnt == PKT_NUM) {
      break;
    }
  }
  printf("Reading pkt trace done!\n");
  printf("average pkt size = %lf\n", pkt_size * 1.0 / PKT_NUM);
}

pkt_t *next_pkt(uint8_t nf_idx) {
  int zipf_r = popzipf(PKT_NUM, 1.1);
  return pkts + zipf_r;
}

#endif /* __PKT_PULLER_H__ */
