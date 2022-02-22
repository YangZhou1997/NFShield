#include <stdatomic.h>
#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/pkt-header.h"
#include "./utils/raw_icenet.h"
#include "./utils/encoding.h"
#include "./nfs/acl-fw.h"
#include "./nfs/dpi.h"
#include "./nfs/lpm.h"
#include "./nfs/maglev.h"
#include "./nfs/monitoring.h"
#include "./nfs/nat-tcp-v4.h"
#include "./nfs/mem-test.h"
#include "config.h"

int l2_fwd_init(){
    return 0;
}
void l2_fwd(uint8_t *pkt_ptr){
    swap_mac_addr(pkt_ptr);
    return;
}
void l2_fwd_destroy(){
    return;
}

static char nf_names[8][128];
static int (*nf_init[8])();
static void (*nf_process[8])(uint8_t *);
static void (*nf_destroy[8])();

static int num_nfs = 0;
__thread int sockfd  = 0;
pkt_t pkt_buf_all[NCORES][MAX_BATCH_SIZE];

void *loop_func(void *arg){
    int nf_idx = *(int*)arg;
    pkt_t* pkt_buf = pkt_buf_all[nf_idx];
    sockfd = nf_idx;

    nic_boot_pkt(nf_idx);

    if(nf_init[nf_idx]() < 0){
        printf("nf_init error, exit\n");
        exit(0);
    }

    int numpkts = 0;
    uint64_t start = rdcycle(); 
    uint64_t pkt_size_sum = 0;
    uint32_t pkt_num = 0;
    while(1){
        numpkts = recvfrom_batch(sockfd, BUF_SIZ, pkt_buf);
        if(numpkts <= 0){
            continue;
        }
        // printf("[loop_func %d] receiving numpkts %d\n", nf_idx, numpkts);
        for(int i = 0; i < numpkts; i++){
            nf_process[nf_idx](pkt_buf[i].content + NET_IP_ALIGN);
         
            pkt_size_sum += pkt_buf[i].len;
            pkt_num ++;

            if(pkt_num % PRINT_INTERVAL == 0){
                printf("%-12s (nf_idx %u): pkts received %u, avg_pkt_size %lf\n", nf_names[nf_idx], nf_idx, pkt_num, pkt_size_sum * 1.0 / pkt_num);
            }
            if(pkt_num >= TEST_NPKTS){
                double time_taken = (rdcycle() - start) / CPU_GHZ * 1e-3; 
                printf("%-12s (nf_idx %u): processed pkts %u, elapsed time (us) %lf, processing rate %.8lf Mpps\n", 
                    nf_names[nf_idx], nf_idx, TEST_NPKTS, time_taken, (double)(TEST_NPKTS)/time_taken);
                // stopping the pktgen. 
                struct tcp_hdr * tcph = (struct tcp_hdr *) (pkt_buf[i].content + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));
                tcph->recv_ack = 0xFFFFFFFF;
                goto finished;
            }
        }
        sendto_batch(sockfd, numpkts, pkt_buf);
    }
finished:
    sendto_batch(sockfd, numpkts, pkt_buf);
    nf_destroy[nf_idx]();
    return NULL;
}

#define MALLOC_SIZE ((uint64_t)(1.5 * 1024 * 1024 * 1024))
static uint8_t malloc_bytes[MALLOC_SIZE];

void thread_entry(int cid, int nc){
    malloc_addblock(malloc_bytes, MALLOC_SIZE);
    
    nf_process[0] = l2_fwd;
    nf_process[1] = acl_fw;
    nf_process[2] = dpi;
    nf_process[3] = lpm;
    nf_process[4] = maglev;
    nf_process[5] = monitoring;
    nf_process[6] = nat_tcp_v4;
    nf_process[7] = mem_test;

    nf_init[0] = l2_fwd_init;
    nf_init[1] = acl_fw_init;
    nf_init[2] = dpi_init;
    nf_init[3] = lpm_init;
    nf_init[4] = maglev_init;
    nf_init[5] = monitoring_init;
    nf_init[6] = nat_tcp_v4_init;
    nf_init[7] = mem_test_init;

    nf_destroy[0] = l2_fwd_destroy;
    nf_destroy[1] = acl_fw_destroy;
    nf_destroy[2] = dpi_destroy;
    nf_destroy[3] = lpm_destroy;
    nf_destroy[4] = maglev_destroy;
    nf_destroy[5] = monitoring_destroy;
    nf_destroy[6] = nat_tcp_v4_destroy;
    nf_destroy[7] = mem_test_destroy;

    char* nf_names[8] = {"l2_fwd", "acl_fw", "dpi", "lpm", "maglev", "monitoring", "nat_tcp_v4", "mem_test"};
    for (int i = 0; i < 8; i++) {
        char* nf_name = nf_names[i];
        if (strstr(NF_STRINGS, nf_name)) {
            nf_process[num_nfs] = nf_process[i];
            nf_init[num_nfs] = nf_init[i];
            nf_destroy[num_nfs] = nf_destroy[i];
            nf_names[num_nfs] = nf_names[i];
            num_nfs++;
            // printf("Register NF: %s\n", nf_name);
        }
    }

    if (num_nfs > 4) {
       printf("Only support num_nfs <= 4!");
       exit(0);
    }
    
    printf("%d NFs: ", num_nfs);
    for(int i = 0; i < num_nfs; i++){
        printf("%s ", nf_names[i]);
    }
    printf("\n");
    
    int nf_idx[4] = {0, 1, 2, 3};
    loop_func((void*)(nf_idx + cid));
}