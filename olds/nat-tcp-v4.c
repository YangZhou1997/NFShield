#include <stdio.h>
#include <stdlib.h>
#define VALUE_TYPE TUPLE

#include "../utils/common.h"
#include "../utils/pkt-ops.h"
#include "../utils/common.h"
// #define VALUE_TYPE TUPLE
#define TYPE five_tuple_t
#define TYPE_STR TUPLE
#define TYPED_NAME(x) tuple_##x
#include "../utils/dleft-hash.h"
#undef TYPE
#undef TYPE_STR
#undef TYPED_NAME
#include "../utils/pkt-puller.h"

#define HT_SIZE_NAT (1.6 * 1024 * 1024)
static tuple_dleft_meta_t ht_meta;


int main(){

    if(-1 == tuple_dleft_init("nat-tcp-v4", HT_SIZE_NAT, &ht_meta))
    {
        printf("bootmemory allocation error\n");
        return 0;
    }
    
    srand((unsigned)time(NULL));
   
    load_pkt("../data/ictf2010_100kflow.dat");
    int FAU_PORTS = 0;
    uint32_t pkt_cnt = 0;
    while(1){
        pkt_t *raw_pkt = next_pkt();
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        swap_mac_addr(pkt_ptr);

        five_tuple_t flow;
        get_five_tuple(pkt_ptr, &flow);

// #define DEBUG
#ifdef DEBUG
        uint64_t src_mac = get_src_mac(pkt_ptr);
        uint64_t dst_mac = get_dst_mac(pkt_ptr);
#endif

#ifdef DEBUG
        printf("%lx %lx %x %x %hu %hu %hu\n", src_mac, dst_mac, flow.srcip, flow.dstip, flow.srcport, flow.dstport, flow.proto);
#endif

// #define DEBUG

        five_tuple_t * lookup_value;
        lookup_value = tuple_dleft_lookup(&ht_meta, flow);

        if(lookup_value == NULL)
        {
            uint32_t cur_port = FAU_PORTS ++;
            if(cur_port <= 65535)  
            {
                set_src_ip(pkt_ptr, 0x10000001);
                set_src_port(pkt_ptr, cur_port);

                five_tuple_t outgoing_flow = flow;
                outgoing_flow.srcip = 0x10000001;
                outgoing_flow.srcport = (uint16_t)cur_port;

                five_tuple_t rev_flow = REVERSE(outgoing_flow);
                five_tuple_t rev_flow_2 = REVERSE(flow);

                tuple_dleft_update(&ht_meta, flow, outgoing_flow);
                tuple_dleft_update(&ht_meta, rev_flow, rev_flow_2);
            }
        }
        else
        {
            set_five_tuple(pkt_ptr, *lookup_value);
        }

        pkt_cnt ++;
         if(pkt_cnt % PRINT_INTERVAL == 0) {
             printf("nat-tcp-v4 %u\n", pkt_cnt);
         }
    }    
    tuple_dleft_destroy(&ht_meta);

    return 0;
}
