#include <stdio.h>
#include <stdlib.h>

#include "../utils/common.h"
#include "../utils/pkt-ops.h"
#include "../utils/common.h"
#include "../utils/lpm-algo.h"
#include "../utils/pkt-puller.h"

static iplookup_t *iplookup;
static uint32_t gate_count[GATE_NUM];

int main(){
    if(NULL == (iplookup = zmalloc(sizeof(iplookup_t), ALIGN, NULL)))
    {
        printf("bootmemory allocation error\n");
        return 0;
    }
    srand((unsigned)time(NULL));

#define RAND8 (rand() & 0xFF)
#define TOIP(a, b, c, d) ((a<<24) | (b<<16) | (c<<8) | d)

    for(int i = 0; i < 16000; i++)
    {
        uint32_t ip = TOIP(RAND8, RAND8, RAND8, RAND8);
        uint16_t gate = RAND8;
        lpm_insert(iplookup, ip, 32, gate);
    }
    lpm_construct_table(iplookup);
    
    load_pkt("../data/ictf2010_100kflow.dat");
    uint32_t pkt_cnt = 0;
    while(1){
        pkt_t *raw_pkt = next_pkt(0);
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        swap_mac_addr(pkt_ptr);
        uint32_t srcip = get_src_ip(pkt_ptr);
        uint16_t gate = lpm_lookup(iplookup, srcip);
        gate_count[gate] ++;
        pkt_cnt ++;
         if(pkt_cnt % PRINT_INTERVAL == 0) {
             printf("lpm %u\n", pkt_cnt);
         }
    }
    zfree(iplookup);

    return 0;
}
