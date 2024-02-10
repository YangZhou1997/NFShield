#include <stdio.h>
#include <stdlib.h>

#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/common.h"
#include "./utils/maglev-algo.h"
#include "./utils/pkt-puller.h"

static maglev_meta_t mag_meta;

int main(){

#define M 65537
#define N 3
    uint32_t backend_ip[N];
    uint8_t backend_status[N];

    for(int i = 0; i < N; i++)
    {
        backend_ip[i] = rand();
        backend_status[i] = HEALTHY;
    }
    maglev_init("maglev", M, N, backend_ip, backend_status, &mag_meta);
 
    srand((unsigned)time(NULL));
   
    load_pkt("/users/yangzhou/NFShield/data/ictf2010_100kflow.dat");
    uint32_t pkt_cnt = 0;
    while(1){
        pkt_t *raw_pkt = next_pkt();
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        if (pkt_ptr == NULL || pkt_len < 54 || pkt_len > 1500) continue;
        swap_mac_addr(pkt_ptr);

        five_tuple_t key;
        get_five_tuple(pkt_ptr, &key);
        uint32_t lookup_value;
        lookup_value = maglev_get_backend(&mag_meta, key);
        set_dst_ip(pkt_ptr, lookup_value);

        pkt_cnt ++;
        // if(pkt_cnt % PRINT_INTERVAL == 0) {
        //     printf("maglev %u\n", pkt_cnt);
        // }
    }    
    maglev_destory(&mag_meta);

    return 0;
}