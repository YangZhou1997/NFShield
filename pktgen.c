#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>

#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/pkt-header.h"
// #include "./utils/pkt-puller.h" // zipf with on-flying seq gen; take small amount of memory (100k distinct packets)
// #include "./utils/pkt-puller2.h" // preloaded normal or zipf traffic; take a lot of memory (2m packets)
#include "./utils/pkt-puller3.h" // zipf with preloaded seq, a bit faster than the first; modest memory (100k distinct packets, and 2m seq number)
#include "./utils/raw_icenet.h"
#include "config.h"

#define MAX_WAITING_CYCLES 2999538 // this is from empirical tests 

static atomic_ullong unack_pkts[4] = {0,0,0,0};
static atomic_ullong sent_pkts[4] = {0,0,0,0};
static atomic_ullong sent_pkts_size[4] = {0,0,0,0};
static atomic_ullong received_pkts[4] = {0,0,0,0};
static atomic_ullong lost_pkts[4] = {0,0,0,0};

static atomic_uchar force_quit[4];
static atomic_uchar print_order = 0;
static atomic_uchar finished_nfs = 0;
static atomic_ullong invalid_pkts = 0;

__thread pkt_t* pkt_buf_recv[MAX_BATCH_SIZE];
__thread uint8_t warmup_end_recv = 0;
__thread uint64_t lost_pkt_during_cold_start;

static int num_nfs = 1;

void try_recv_pkt(int nf_idx){
    // try to receive a batch of packets. This call is non-blocking
    int numpkts_recv = recvfrom_batch(nf_idx, BUF_SIZ, pkt_buf_recv);
    if(numpkts_recv <= 0){
        return;
    }
    for(int i = 0; i < numpkts_recv; i++){
    	struct ether_hdr *eh_recv = (struct ether_hdr *) pkt_buf_recv[i]->content;
    	struct tcp_hdr * tcph_recv = (struct tcp_hdr *) (pkt_buf_recv[i]->content + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));
        int recv_nf_idx = (int)htons((eh_recv->ether_type)) - CUSTOM_PROTO_BASE;

        // printf("[recv_pacekts %d] recv_nf_idx = %d, tcph->sent_seq = %x\n", nf_idx, recv_nf_idx, tcph->sent_seq);

        if(tcph_recv->sent_seq != 0xdeadbeef){
            invalid_pkts ++;
            continue;
        }
        if(recv_nf_idx < 4)
            received_pkts[recv_nf_idx] ++;

        if(nf_idx == recv_nf_idx){
            uint32_t pkt_idx = tcph_recv->recv_ack;
            uint64_t curr_received_pkts = received_pkts[nf_idx];
            uint32_t lost_pkts = pkt_idx + 1 - curr_received_pkts;

            // TODO: the packets might get re-ordered. 
            // printf("%lu, %llu\n", pkt_idx, curr_received_pkts);
            
            // these packets are lost
            // if(lost_pkts != 0){
            //     received_pkts[nf_idx] += lost_pkts
            // }

            if(!warmup_end_recv && pkt_idx >= WARMUP_NPKTS - 1)
            {
        		warmup_end_recv = 1;
                lost_pkt_during_cold_start = lost_pkts; 
                printf("[recv_pacekts th%d]: warm up ends, lost_pkt_during_cold_start = %llu\n", nf_idx, lost_pkt_during_cold_start);
        		// printf("[recv_pacekts th%d] pkt_idx in latest packet %u, received_pkts[nf_idx] %llu\n", nf_idx, pkt_idx, curr_received_pkts);
            }
            if(curr_received_pkts % PRINT_INTERVAL == 0){
                printf("[recv_pacekts th%d]: pkts received: %llu\n", nf_idx, curr_received_pkts);
            }

            if(tcph_recv->recv_ack == 0xFFFFFFFF){
                return;
            }
        }
    }
}

pkt_t* pkt_buf_all[NCORES][MAX_BATCH_SIZE];
void *send_pkt_func(void *arg) {
	// which nf's traffic this thread sends
    int nf_idx = *(int*)arg;
    pkt_t** pkt_buf = pkt_buf_all[nf_idx];
    
    // sending-related variables        
    uint64_t* pkt_bytes;
    for(int i = 0; i < PKT_NUM; i++){
        pkt_bytes = (uint64_t*)pkts[i].content;
        pkt_bytes[0] = MAC_NFTOP << 16;
        pkt_bytes[1] = MAC_PKTGEN | ((uint64_t)htons(ETH_P_IP) << 48);
    }

    struct timeval start, end; 
    uint64_t burst_size;
    struct tcp_hdr *tcph;
    uint8_t warmup_end = 0;

    struct ether_hdr* eh;

    gettimeofday(&start, NULL);

    // uint32_t max_waiting_cycles = 0;
    uint32_t waiting_cycles = 0;
    while(sent_pkts[nf_idx] < TEST_NPKTS + WARMUP_NPKTS){
        while(sent_pkts[nf_idx] >= received_pkts[nf_idx] && (unack_pkts[nf_idx] = sent_pkts[nf_idx] - received_pkts[nf_idx]) >= MAX_UNACK_WINDOW){
            try_recv_pkt(nf_idx);
            // ad-hoc solution to break deadlock caused by packet loss -- should rarely happen
            waiting_cycles ++;
            if(waiting_cycles > MAX_WAITING_CYCLES){
                lost_pkts[nf_idx] += sent_pkts[nf_idx] - received_pkts[nf_idx];
                sent_pkts[nf_idx] = received_pkts[nf_idx];
                printf("[send_pacekts th%d]: deadlock detected (caused by packet loss or NF initing), forcely resolving...\n", nf_idx);
            }
            if(force_quit[nf_idx])
				break;
        }
        // max_waiting_cycles = MAX(max_waiting_cycles, waiting_cycles);
        waiting_cycles = 0;

        burst_size = MAX_UNACK_WINDOW - unack_pkts[nf_idx];
        burst_size = MIN(burst_size, TEST_NPKTS + WARMUP_NPKTS - sent_pkts[nf_idx]);

        for(int i = 0; i < burst_size; i++){
            pkt_buf[i] = next_pkt(nf_idx);
        	
            eh = (struct ether_hdr*)pkt_buf[i]->content;
            eh->ether_type = htons((uint16_t)nf_idx + CUSTOM_PROTO_BASE);

            tcph = (struct tcp_hdr *) (pkt_buf[i]->content + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));
            tcph->sent_seq = 0xdeadbeef;
            tcph->recv_ack = sent_pkts[nf_idx] + i;
            // inter-pkt frame
            sent_pkts_size[nf_idx] += pkt_buf[i]->len + 20;
        }

        sendto_batch(nf_idx, burst_size, pkt_buf);
        sent_pkts[nf_idx] += burst_size;

        try_recv_pkt(nf_idx);

        if(!warmup_end && sent_pkts[nf_idx] >= WARMUP_NPKTS){
            gettimeofday(&start, NULL);
            warmup_end = 1; 
            sent_pkts_size[nf_idx] = 0;
        }
        if((sent_pkts[nf_idx] / MAX_BATCH_SIZE) % (PRINT_INTERVAL / MAX_BATCH_SIZE) == 0){
            printf("[send_pacekts th%d]:     pkts sent: %llu, unacked pkts: %llu, potentially lost pkts: %llu\n", nf_idx, sent_pkts[nf_idx], unack_pkts[nf_idx], lost_pkts[nf_idx]);
        }

       if(force_quit[nf_idx])
			break;
    }
    gettimeofday(&end, NULL); 
    if(warmup_end){
        sent_pkts[nf_idx] -= WARMUP_NPKTS;
    }
    double time_taken; 
    time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;

	close(nf_idx);

    finished_nfs++;
    while(print_order != nf_idx || finished_nfs != num_nfs){
        sleep(1);
    }
    printf("[send_pacekts th%d]:     pkts sent: %llu, unacked pkts: %4llu, potentially lost pkts: %4llu, %.8lf Mpps, %.6lfGbps\n", nf_idx, sent_pkts[nf_idx], unack_pkts[nf_idx], lost_pkts[nf_idx], (double)(sent_pkts[nf_idx]) * 1e-6 / time_taken, (double)sent_pkts_size[nf_idx] * 8 / time_taken * 1e-9);
    // printf("max_waiting_cycles = %u\n", max_waiting_cycles);
    print_order = nf_idx + 1;
   
    while(print_order != num_nfs){
        sleep(1);
    }
    exit(0);
    // any of the following two will crash. ?? why
    // pthread_exit(arg);
    // return NULL;
    // pthread_exit(NULL);
}

void thread_entry(int cid, int nc){
    for(int i = 0; i < 4; i++){
		force_quit[i] = 0;
    }

    // for pkt_puller2;
    // char * tracepath = "./data/ictf2010_2mpacket.dat";
    // char * tracepath = "./data/ictf2010_2mpacket_zipf.dat"
    // for pkt_puller and pkt_puller3;
    char * tracepath = "./data/ictf2010_100kflow.dat";
    
    num_nfs = NUM_NFS;

    if (num_nfs > 4) {
       printf("Only support num_nfs <= 4!");
       exit(0);
    }
    if (cid > num_nfs) {
        return;
    }
    
    // only one core needs to load the trace
    load_pkt(tracepath);

    int nf_idx[4] = {0, 1, 2, 3};
    send_pkt_func((void*)(nf_idx + cid));
}
    