#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "../utils/common.h"
#include "../utils/pkt-ops.h"
#include "../utils/common.h"
#include "../utils/pkt-puller.h"
#include "../utils/search_ac2.h"
// #define AC_DUMP

static ACSM_STRUCT2 *acsm;
static uint32_t match_total_dpi = 0;
static uint32_t pkt_cnt_dpi = 0;

int dpi_init(){
    acsm = acsmNew2(NULL, NULL, NULL);
    if (!acsm){
        printf("acsm init failed\n");
        return -1;
    }
    acsm->acsmFormat = ACF_BANDED;
    
    match_total_dpi = 0;

// #define AC_DUMP
#ifdef AC_DUMP
    // You must use absolute address, otherwise gem5 will show "Page table fault when accessing virtual address 0"
    FILE * file_rule = fopen("./data/sentense.rules", "r");
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

    FILE * f_dump = fopen("./data/ac.raw", "w");
    acsmDumpSparseDFA_Banded(acsm, f_dump);
#else
    FILE * f_restore = fopen("./data/ac.raw", "r");
    acsmRestoreSparseDFA_Banded(acsm, f_restore);
    printf("ac restore done!\n");
#endif

    srand((unsigned)time(NULL));
    return 0;
}

void dpi(uint8_t *pkt_ptr){
    struct ipv4_hdr *iph = (struct ipv4_hdr *) (pkt_ptr + sizeof(struct ether_hdr));
    uint16_t pkt_len = htons(((struct ipv4_hdr *)iph)->total_length) + sizeof(struct ether_hdr);

    swap_mac_addr(pkt_ptr);

    int cur = 0;
    acsmSearch2(acsm, (unsigned char *)(pkt_ptr + 54), pkt_len - 54, MatchFound, (void *)&match_total_dpi, &cur);

    pkt_cnt_dpi ++;
    // if(pkt_cnt_dpi % PRINT_INTERVAL == 0) {
    //     printf("dpi %u\n", pkt_cnt_dpi);
    //     // break;
    // }
}

void dpi_destroy(){
    printf("match_total_dpi: %u\n", match_total_dpi);
    acsmFree2(acsm);
}
