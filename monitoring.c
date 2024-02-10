#include <stdio.h>
#include <stdlib.h>
#define VALUE_TYPE U32

#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/common.h"
#include "./utils/dleft-hash.h"
#include "./utils/pkt-puller.h"

#define HT_SIZE (0.5 * 1024 * 1024)
static dleft_meta_t ht_meta;

int main(){

    if(-1 == dleft_init("monitoring", HT_SIZE, &ht_meta))
    {
        printf("bootmemory allocation error\n");
        return 0;
    }

    srand((unsigned)time(NULL));
   
    load_pkt("/users/yangzhou/NFShield/data/ictf2010_100kflow.dat");
    uint32_t pkt_cnt = 0;
    while(1){
        pkt_t *raw_pkt = next_pkt();
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        if (pkt_ptr == NULL || pkt_len < 54 || pkt_len > 1500) continue;
        swap_mac_addr(pkt_ptr);

        five_tuple_t flow;
        get_five_tuple(pkt_ptr, &flow);
        dleft_add_value(&ht_meta, flow, 1);

        pkt_cnt ++;
        // if(pkt_cnt % PRINT_INTERVAL == 0) {
        //     printf("monitoring %u\n", pkt_cnt);
        // }
    }    
    dleft_destroy(&ht_meta);

    return 0;
}