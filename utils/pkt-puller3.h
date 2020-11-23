#ifndef __PKT_PULLER_H__
#define __PKT_PULLER_H__

#ifdef	__cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "pkt-header.h"
#include "zipf-gen.h"

#define PKT_NUM (100 * 1024)
#define TOTAL_SEQ (1 << 21) // 2*1024*1024

pkt_t pkts[PKT_NUM];
uint32_t seqs[TOTAL_SEQ];

void load_pkt(char *filename){
    if(strcmp(filename, "./data/ictf2010_100kflow.dat") != 0){
        printf("pkt_puller3 must use trace of ./data/ictf2010_100kflow.dat\n");
        exit(0);
    }

    printf("trying to open file %s\n", "./data/ictf2010_100kflow.dat");
    FILE * file = fopen("./data/ictf2010_100kflow.dat", "r");

    printf("trying to open file %s\n", "./data/ictf2010_100kflow_2mseq.dat");
    FILE * file_seq = fopen("./data/ictf2010_100kflow_2mseq.dat", "r");
    
    if(file == NULL || file_seq == NULL){
        printf("file open error\n");
        exit(0);
    }
    printf("opening succeeds\n");

    uint32_t pkt_cnt = 0;
    uint32_t pkt_size = 0;
    uint32_t seq = 0;
    while(1){
        fread(&pkts[pkt_cnt].len, sizeof(uint16_t), 1, file);
        pkt_size += pkts[pkt_cnt].len;
        pkts[pkt_cnt].content = (uint8_t *)malloc(pkts[pkt_cnt].len);
        fread(pkts[pkt_cnt].content, 1, pkts[pkt_cnt].len, file);
        pkt_cnt ++;
        if(pkt_cnt == PKT_NUM){
            break;
        }
    }
    uint32_t cnt = 0;
    while(cnt < TOTAL_SEQ){
        fread(&seqs[cnt], sizeof(uint32_t), 1, file_seq);
        cnt ++;
    }
    fclose(file);
    fclose(file_seq);
    printf("Reading pkt trace done!\n");
    printf("average pkt size = %lf\n", pkt_size * 1.0 / PKT_NUM);
}

uint32_t seq_idx[7] = {0};
pkt_t *next_pkt(uint8_t nf_idx){
    pkt_t* curr_pkt = pkts + seqs[seq_idx[nf_idx]];
    seq_idx[nf_idx] = (seq_idx[nf_idx] + 1) & (TOTAL_SEQ - 1);
    return curr_pkt;
}

#ifdef	__cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif /* __PKT_PULLER_H__ */
