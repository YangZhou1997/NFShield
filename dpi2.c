#include <stdio.h>
#include <stdlib.h>

#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/common.h"
#include "./utils/ahocorasick.h"
#include "./utils/pkt-puller.h"

int main(){

    struct ahocorasick aho;
    uint32_t match_total = 0;

    aho_init(&aho);

    // You must use absolute address, otherwise gem5 will show "Page table fault when accessing virtual address 0"
    FILE * file_rule = fopen("/users/yangzhou/NF-GEM5/sentense.rules", "r");
    char rule_buf[1024];
    int cnt = 0, len = 0;

    while(fgets(rule_buf, 1023, file_rule)) {
        len = strlen(rule_buf);
        rule_buf[len - 1] = '\0';
        // printf("%d: %s\n", len, rule_buf);
        aho_add_match_text(&aho, rule_buf, len);
        cnt ++;
    }
    printf("dpi rules length: %d\n", cnt);
    aho_create_trie(&aho);
    printf("dpi AhoCorasick graph built up!\n");

    srand((unsigned)time(NULL));
   
    load_pkt("/users/yangzhou/ictf2010_100kflow.dat");
    uint32_t pkt_cnt = 0;
    while(1){
        pkt_t *raw_pkt = next_pkt();
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        swap_mac_addr(pkt_ptr);

        match_total += aho_findtext(&aho, pkt_ptr + 54, pkt_len - 54);

        pkt_cnt ++;
        // if(pkt_cnt % PRINT_INTERVAL == 0) {
        //     printf("dpi %u\n", pkt_cnt);
        // }
    }    
    aho_destroy(&aho);

    return 0;
}