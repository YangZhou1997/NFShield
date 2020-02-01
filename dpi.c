#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/common.h"
#include "./utils/pkt-puller.h"
#include "./utils/search_ac2.h"

// #define AC_DUMP

int main(){
    
    ACSM_STRUCT2 *acsm = acsmNew2(NULL, NULL, NULL);
    if (!acsm){
        printf("acsm init failed\n");
        return -1;
    }
    acsm->acsmFormat = ACF_BANDED;
    
    uint32_t match_total = 0;

#ifdef AC_DUMP
    // You must use absolute address, otherwise gem5 will show "Page table fault when accessing virtual address 0"
    FILE * file_rule = fopen("/users/yangzhou/NF-GEM5/sentense.rules", "r");
    char rule_buf[1024];
    int cnt = 0, len = 0;
    while(fgets(rule_buf, 1023, file_rule)) {
        len = strlen(rule_buf);
        rule_buf[len - 1] = '\0';
        // printf("%d: %s\n", len, rule_buf);
        acsmAddPattern2(acsm, (unsigned char*)rule_buf, strlen(rule_buf), 1/*no case*/,
            0, 0, 0, (void*)NULL, cnt);
        cnt ++;
        // if(cnt >= 1024){
        //     break;
        // }
    }
    printf("dpi rules length: %d\n", cnt);

    acsmCompile2(acsm, NULL, NULL);
    acsmPrintInfo2(acsm);    
    printf("dpi AhoCorasick graph built up!\n");

    FILE * f_dump = fopen("/users/yangzhou/ac.raw", "w");
    acsmDumpSparseDFA_Banded(acsm, f_dump);
#else
    FILE * f_restore = fopen("/users/yangzhou/ac.raw", "r");
    acsmRestoreSparseDFA_Banded(acsm, f_restore);
    printf("ac restore done!\n");
#endif

    srand((unsigned)time(NULL));
   
    load_pkt("/users/yangzhou/ictf2010_100kflow.dat");
    uint32_t pkt_cnt = 0;
    while(1){
        pkt_t *raw_pkt = next_pkt();
        uint8_t *pkt_ptr = raw_pkt->content;
        uint16_t pkt_len = raw_pkt->len;
        swap_mac_addr(pkt_ptr);

        int cur = 0;
        acsmSearch2(acsm, (unsigned char *)(pkt_ptr + 54), pkt_len - 54, MatchFound, (void *)&match_total, &cur);

        pkt_cnt ++;
        if(pkt_cnt % PRINT_INTERVAL == 0) {
            printf("dpi %u\n", pkt_cnt);
            // break;
        }
    }    
    printf("match_total: %u\n", match_total);
#ifdef AC_DUMP
    // acsmFree2(acsm);
#endif
    return 0;
}