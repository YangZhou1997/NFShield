#include <stdio.h>
#include <stdlib.h>
#define VALUE_TYPE U32

#include "../utils/common.h"
#include "../utils/pkt-ops.h"
// #define VALUE_TYPE U32
#define TYPE uint32_t
#define TYPE_STR U32
#define TYPED_NAME(x) u32_##x
#include "../utils/dleft-hash.h"
#undef TYPE
#undef TYPE_STR
#undef TYPED_NAME
#include "../utils/pkt-puller.h"

#define HT_SIZE_MON (0.5 * 1024 * 1024)
static u32_dleft_meta_t ht_meta;

int main(){

    if(-1 == u32_dleft_init("monitoring", HT_SIZE_MON, &ht_meta))
    {
        printf("bootmemory allocation error\n");
        return 0;
    }

    srand((unsigned)time(NULL));
   
    load_pkt("../data/ictf2010_100kflow.dat");
    uint32_t pkt_cnt = 0;
    while(1){
        pkt_t *raw_pkt = next_pkt();
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        swap_mac_addr(pkt_ptr);

        five_tuple_t flow;
        get_five_tuple(pkt_ptr, &flow);
        u32_dleft_add_value(&ht_meta, flow, 1);

        pkt_cnt ++;
         if(pkt_cnt % PRINT_INTERVAL == 0) {
             printf("monitoring %u\n", pkt_cnt);
         }
    }    
    u32_dleft_destroy(&ht_meta);

    return 0;
}
