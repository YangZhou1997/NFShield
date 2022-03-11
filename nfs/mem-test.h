#include <stdio.h>
#include <stdlib.h>

#include "../utils/common.h"
#include "../utils/pkt-ops.h"

static uint32_t pkt_cnt_mem_test = 0;
static uint32_t total_bytes = 2*1024*1024;
static uint32_t access_bytes_per_pkt = 1024;

static uint8_t* array = NULL;

int mem_test_init(){
    array = (uint8_t*)malloc(total_bytes);
    if(!array)
    {
        printf("bootmemory allocation error\n");
        return 0;
    }
    srand((unsigned)time(NULL));
    printf("mem_test init done!\n");
    return 0;
}
void mem_test(uint8_t *pkt_ptr){
    swap_mac_addr(pkt_ptr);

    uint32_t start = rand() % (total_bytes - access_bytes_per_pkt);
    for(uint32_t i = start; i < start + access_bytes_per_pkt - sizeof(uint32_t); i += sizeof(uint32_t)){
        *(uint32_t*)(array + i) += 1;
    }

    pkt_cnt_mem_test ++;
    // if(pkt_cnt_mem_test % PRINT_INTERVAL == 0) {
    //     printf("mem_test %u\n", pkt_cnt_mem_test);
    // }
}
void mem_test_destroy(){
    free(array);
}