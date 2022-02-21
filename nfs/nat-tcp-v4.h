#include <stdio.h>
#include <stdlib.h>

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

#define HT_SIZE_NAT (1.6 * 1024 * 1024)
static tuple_dleft_meta_t ht_meta_nat;
static uint32_t pkt_cnt_nat = 0;
static int FAU_PORTS = 0;

int nat_tcp_v4_init(){
    if(-1 == tuple_dleft_init("nat-tcp-v4", HT_SIZE_NAT, &ht_meta_nat))
    {
        printf("bootmemory allocation error\n");
        return 0;
    }
    srandom((unsigned)time(NULL));
    printf("nat init done!\n");
    return 0;
}
void nat_tcp_v4(uint8_t *pkt_ptr){
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
    lookup_value = tuple_dleft_lookup(&ht_meta_nat, flow);

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

            tuple_dleft_update(&ht_meta_nat, flow, outgoing_flow);
            tuple_dleft_update(&ht_meta_nat, rev_flow, rev_flow_2);
        }
    }
    else
    {
        set_five_tuple(pkt_ptr, *lookup_value);
    }

    pkt_cnt_nat ++;
    // if(pkt_cnt_nat % PRINT_INTERVAL == 0) {
    //     printf("nat-tcp-v4 %u\n", pkt_cnt_nat);
    // }
}
void nat_tcp_v4_destroy(){
    tuple_dleft_destroy(&ht_meta_nat);
}
