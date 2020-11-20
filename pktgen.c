#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <pthread.h> 
#include <signal.h>
#include <sys/time.h> 
#include <unistd.h>

#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/pkt-header.h"
#include "./utils/pkt-puller.h"
// #include "./utils/pkt-puller2.h"
#include "./utils/raw_socket.h"

#define MIN(x, y) (x < y ? x : y)

#define MAX_UNACK_WINDOW 128
static volatile uint64_t unack_pkts[4] = {0,0,0,0};
static volatile uint64_t sent_pkts[4] = {0,0,0,0};
static volatile uint64_t received_pkts[4] = {0,0,0,0};

static volatile uint8_t force_quit_send;
static volatile uint8_t force_quit_recv;

#define TEST_NPKTS (2*1024*1024)
#define WARMUP_NPKTS (1*1024*1024)

static int sockfd  = 0;
static struct sockaddr_ll send_sockaddr;
static struct ifreq if_mac;

void *send_pkt_func(void *arg) {
	// which nf's traffic this thread sends
    int nf_idx = *(int*)arg;
    
    struct ether_hdr* eh;
    for(int i = 0; i < PKT_NUM; i++){
        eh = (struct ether_hdr*) pkts[i].content;
        /* Ethernet header */
        eh->s_addr.addr_bytes[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
    	eh->s_addr.addr_bytes[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
    	eh->s_addr.addr_bytes[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
    	eh->s_addr.addr_bytes[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
    	eh->s_addr.addr_bytes[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
    	eh->s_addr.addr_bytes[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
    	eh->d_addr.addr_bytes[0] = MY_DEST_MAC0;
    	eh->d_addr.addr_bytes[1] = MY_DEST_MAC1;
    	eh->d_addr.addr_bytes[2] = MY_DEST_MAC2;
    	eh->d_addr.addr_bytes[3] = MY_DEST_MAC3;
    	eh->d_addr.addr_bytes[4] = MY_DEST_MAC4;
    	eh->d_addr.addr_bytes[5] = MY_DEST_MAC5;
    	/* Ethertype field */
    	eh->ether_type = htons(ETH_P_IP);
    }

    struct timeval start, end; 
    uint64_t burst_size;
    struct tcp_hdr *tcph;
    uint8_t warmup_end = 0;
    gettimeofday(&start, NULL);
    while(sent_pkts[nf_idx] < TEST_NPKTS + WARMUP_NPKTS){
        while(sent_pkts[nf_idx] >= received_pkts[nf_idx] && (unack_pkts[nf_idx] = sent_pkts[nf_idx] - received_pkts[nf_idx]) >= MAX_UNACK_WINDOW){
            if(force_quit_send)
				break;
        }
        burst_size = MAX_UNACK_WINDOW - unack_pkts[nf_idx];
        burst_size = MIN(burst_size, TEST_NPKTS + WARMUP_NPKTS - sent_pkts[nf_idx]);

        for(int i = 0; i < burst_size; i++){
            pkt_t *raw_pkt = next_pkt();
        	tcph = (struct tcp_hdr *) (raw_pkt->content + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));
            tcph->sent_seq = 0xdeadbeef;
            tcph->recv_ack = sent_pkts[nf_idx] + i;
            eh = (struct ether_hdr*)raw_pkt->content;
            eh->d_addr.addr_bytes[5] = (uint8_t)nf_idx;

        	/* Send packet */
        	if (sendto(sockfd, raw_pkt->content, raw_pkt->len, 0, (struct sockaddr*)&send_sockaddr, sizeof(struct sockaddr_ll)) < 0){
        	    printf("[send_pacekts %d] send failed\n", nf_idx);
            }
            else{
                // sent_pkts[nf_idx] ++;
                __atomic_fetch_add(&sent_pkts[nf_idx], 1, __ATOMIC_SEQ_CST);
            }
 
            if(sent_pkts[nf_idx] == WARMUP_NPKTS){
                gettimeofday(&start, NULL);
                warmup_end = 1; 
            }
            if(sent_pkts[nf_idx] % PRINT_INTERVAL == 0){
                printf("[send_pacekts %d] number of pkts sent: %u\n", nf_idx, sent_pkts[nf_idx]);
            }
        }
       if(force_quit_send)
			break;
    }
    gettimeofday(&end, NULL); 
    if(warmup_end){
        sent_pkts[nf_idx] -= WARMUP_NPKTS;
    }
    double time_taken; 
    time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    printf("[send_pacekts %d]: %llu pkt sent, %.4lf Mpps\n", nf_idx, sent_pkts[nf_idx], (double)(sent_pkts[nf_idx]) * 1e-6 / time_taken);
    
    // ending all packet sending.
    force_quit_send = 1;
    // ending all packet receiving. 
    force_quit_recv = 1;

    return NULL;
}

static volatile uint64_t invalid_pkts = 0;
void * recv_pkt_func(void *arg){
    // which nf's traffic this thread sends
    int nf_idx = *(int*)arg;
    int recv_nf_idx = 0, numbytes = 0;
	uint8_t buf[BUF_SIZ];
	struct ether_hdr *eh = (struct ether_hdr *) buf;
	struct tcp_hdr *tcph = (struct tcp_hdr *) (buf + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));

    uint8_t warmup_end = 0;
    uint64_t lost_pkt_during_cold_start;
	while(!force_quit_recv){
        numbytes = recvfrom(sockfd, buf, BUF_SIZ, MSG_DONTWAIT, NULL, NULL);
        if(numbytes <= 0){
            continue;
        }
        // received nf_idx
        recv_nf_idx = (int)eh->d_addr.addr_bytes[5];
        // printf("[recv_pacekts %d] recv_nf_idx = %d, tcph->sent_seq = %x\n", nf_idx, recv_nf_idx, tcph->sent_seq);

        if(tcph->sent_seq != 0xdeadbeef){
            __atomic_fetch_add(&invalid_pkts, 1, __ATOMIC_SEQ_CST);
            continue;
        }
        else{
            __atomic_fetch_add(&received_pkts[recv_nf_idx], 1, __ATOMIC_SEQ_CST);
        }

        if(nf_idx == recv_nf_idx){
            uint32_t pkt_idx = tcph->recv_ack;
            if(!warmup_end && pkt_idx >= WARMUP_NPKTS - 1)
            {
        		warmup_end = 1;
                lost_pkt_during_cold_start = pkt_idx - received_pkts[nf_idx]; 
                printf("[recv_pacekts %d] warm up ends, lost_pkt_during_cold_start = %llu\n", nf_idx, lost_pkt_during_cold_start);
        		printf("[recv_pacekts %d] pkt_idx %u, received_pkts[nf_idx] %llu\n", nf_idx, pkt_idx, received_pkts[nf_idx]);
        		// fflush(stdout);
            }
            if(received_pkts[nf_idx] % PRINT_INTERVAL == 0){
                printf("[recv_pacekts %d] number of pkts received: %u\n", nf_idx, received_pkts[nf_idx]);
            }
        }
    }
    
	close(sockfd);
    return NULL;
}


static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit_send = 1;
        force_quit_recv = 1;
	}
}

int main(int argc, char *argv[]){
    signal(SIGINT, signal_handler);
    force_quit_recv = 0;
    force_quit_send = 0;

    // char * tracepath = "./data/ictf2010_2mpacket.dat";
    char * tracepath = "./data/ictf2010_100kflow.dat";

    int num_nfs = 1;

    int option;
    while((option = getopt(argc, argv, ":n:t:")) != -1){
        switch (option) {
            case 'n':
                num_nfs = atoi(optarg);
                printf("number of NFs: %d\n", num_nfs);
                break;
            case 't':
                tracepath = optarg;
                printf("trace path: %s\n", tracepath);
             case ':':  
                printf("option -%c needs a value\n", optopt);  
                break;  
            case '?':  
                printf("unknown option: -%c\n", optopt); 
                break; 
        }
    }

    load_pkt(tracepath);
    init_socket(&sockfd, &send_sockaddr, &if_mac);

    pthread_t threads[8];
    int nf_idx[4] = {0, 1, 2, 3};
    for(int i = 0; i < num_nfs*2; i += 2){
        pthread_create(&threads[i], NULL, send_pkt_func, (void*)(nf_idx+i/2));
        pthread_create(&threads[i+1], NULL, recv_pkt_func, (void*)(nf_idx+i/2));
    }
    for(int i = 0; i < num_nfs*2; i += 2){
        pthread_join(threads[i], NULL);
        pthread_join(threads[i+1], NULL);
    }
    
    return 0;
}
    