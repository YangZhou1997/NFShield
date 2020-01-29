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
#include "pkt-header.h"
#include "zipf-gen.h"

#define PKT_NUM (100 * 1024)

typedef struct _pkt
{
    uint16_t len;
    uint8_t * content;
}pkt_t;

pkt_t pkts[PKT_NUM];

void load_pkt(char *filename){
    printf("trying to open file %s\n", filename);
    FILE * file = fopen(filename, "r");
    if(file == NULL){
        printf("file open error\n");
        exit(0);
    }
    printf("opening succeeds\n");
    uint32_t pkt_cnt = 0;
    uint32_t pkt_size = 0;
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
    printf("Reading pkt trace done!\n");
    printf("average pkt size = %lf\n", pkt_size * 1.0 / PKT_NUM);
}

pkt_t *next_pkt(){
    int zipf_r = popzipf(PKT_NUM, 1.1);
    return pkts + zipf_r;
}

#ifdef	__cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif /* __PKT_PULLER_H__ */
