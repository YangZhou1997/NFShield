#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/pkt-header.h"
#include "./utils/raw_socket.h"
#include "./nfs/acl-fw.h"
#include "./nfs/dpi.h"
#include "./nfs/lpm.h"
#include "./nfs/maglev.h"
#include "./nfs/monitoring.h"
#include "./nfs/nat-tcp-v4.h"
#include "./nfs/mem-test.h"
#include "./utils/parsing_mac.h"
#include <stdatomic.h>

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

static uint8_t dstmac[6];
__thread int sockfd  = 0;
__thread struct sockaddr_ll send_sockaddr;
__thread struct ifreq if_mac;

static int num_nfs = 0;
static atomic_uchar print_order = 0;

void *loop_func(void *arg){
    int nf_idx = *(int*)arg;
    set_affinity(nf_idx);
    init_socket(&sockfd, &send_sockaddr, &if_mac, dstmac, nf_idx);
    
    int numpkts = 0;
    struct ether_hdr* eh;
    pkt_t* pkt_buf[MAX_BATCH_SIZE];
    for(int i = 0; i < MAX_BATCH_SIZE; i++){
        pkt_buf[i] = malloc(sizeof(pkt_t));
        pkt_buf[i]->content = malloc(BUF_SIZ);
    }

    if(nf_init[nf_idx]() < 0){
        printf("nf_init error, exit\n");
        exit(0);
    }

    struct timeval start, end; 
    uint64_t pkt_size_sum = 0;
    uint32_t pkt_num = 0;
    while(1){
        numpkts = recvfrom_batch(sockfd, BUF_SIZ, pkt_buf);
        if(numpkts <= 0){
            continue;
        }
        // printf("[loop_func %d] receiving numpkts %d\n", nf_idx, numpkts);
        for(int i = 0; i < numpkts; i++){
            nf_process[nf_idx](pkt_buf[i]->content);
            // eh = (struct ether_hdr*) pkt_buf[i]->content;
         
            pkt_size_sum += pkt_buf[i]->len;
            pkt_num ++;
            if(pkt_num % PRINT_INTERVAL == 0){
                printf("%-12s (nf_idx %u): pkts received %u, avg_pkt_size %lf\n", nf_names[nf_idx], nf_idx, pkt_num, pkt_size_sum * 1.0 / pkt_num);
            }
            if(pkt_num == WARMUP_NPKTS){
                gettimeofday(&start, NULL);
            }
            else if(pkt_num >= WARMUP_NPKTS + TEST_NPKTS){
                gettimeofday(&end, NULL);
                double time_taken; 
                time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
                time_taken = time_taken + (end.tv_usec - start.tv_usec);
                printf("%-12s (nf_idx %u): processed pkts %u, elapsed time (us) %lf, processing rate %.8lf Mpps\n", 
                    nf_names[nf_idx], nf_idx, TEST_NPKTS, time_taken, (double)(TEST_NPKTS)/time_taken);
                // stopping the pktgen. 
                struct tcp_hdr * tcph = (struct tcp_hdr *) (pkt_buf[i]->content + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));
                tcph->recv_ack = 0xFFFFFFFF;
                goto finished;
            }
        }
        sendto_batch(sockfd, numpkts, pkt_buf, &send_sockaddr);
    }
finished:
    sendto_batch(sockfd, numpkts, pkt_buf, &send_sockaddr);
    nf_destroy[nf_idx]();
	close(sockfd);
 
    for(int i = 0; i < MAX_BATCH_SIZE; i++){
        free(pkt_buf[i]->content);
        free(pkt_buf[i]);
    }

    fflush(stdout);
    sleep(1);
    print_order ++;
    // this is used to bypass glibc bugs when calling pthread_join() (https://sourceware.org/bugzilla/show_bug.cgi?id=20116)
    while(print_order != num_nfs){
        sleep(1);
    }
    exit(0);
}

int main(int argc, char* argv[]){
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

    int option;
    while((option = getopt(argc, argv, ":n:d:t:p:")) != -1){
        switch (option) {
            case 'n':
                strcpy(nf_names[num_nfs], optarg);
                if(strcmp("l2_fwd", optarg) == 0){
                    nf_process[num_nfs] = l2_fwd;
                    nf_init[num_nfs] = l2_fwd_init;
                    nf_destroy[num_nfs] = l2_fwd_destroy;
                }
                else if(strcmp("acl_fw", optarg) == 0){
                    nf_process[num_nfs] = acl_fw;
                    nf_init[num_nfs] = acl_fw_init;
                    nf_destroy[num_nfs] = acl_fw_destroy;
                }
                else if(strcmp("dpi", optarg) == 0){
                    nf_process[num_nfs] = dpi;
                    nf_init[num_nfs] = dpi_init;
                    nf_destroy[num_nfs] = dpi_destroy;
                }
                else if(strcmp("lpm", optarg) == 0){
                    nf_process[num_nfs] = lpm;
                    nf_init[num_nfs] = lpm_init;
                    nf_destroy[num_nfs] = lpm_destroy;
                }
                else if(strcmp("maglev", optarg) == 0){
                    nf_process[num_nfs] = maglev;
                    nf_init[num_nfs] = maglev_init;
                    nf_destroy[num_nfs] = maglev_destroy;
                }
                else if(strcmp("monitoring", optarg) == 0){
                    nf_process[num_nfs] = monitoring;
                    nf_init[num_nfs] = monitoring_init;
                    nf_destroy[num_nfs] = monitoring_destroy;
                }
                else if(strcmp("nat_tcp_v4", optarg) == 0){
                    nf_process[num_nfs] = nat_tcp_v4;
                    nf_init[num_nfs] = nat_tcp_v4_init;
                    nf_destroy[num_nfs] = nat_tcp_v4_destroy;
                }
                else if(strcmp("mem_test", optarg) == 0){
                    nf_process[num_nfs] = mem_test;
                    nf_init[num_nfs] = mem_test_init;
                    nf_destroy[num_nfs] = mem_test_destroy;
                }
                else{
                    printf("%s is not a valid nf name\n", optarg);
                    exit(0);
                }
                num_nfs++;
                break;
            case 'd':
                parse_mac(dstmac, optarg);
                printf("dstmac: %02x:%02x:%02x:%02x:%02x:%02x\n", dstmac[0], dstmac[1], dstmac[2], dstmac[3], dstmac[4], dstmac[5]);
                break;
            case 't':
                total_bytes = atoi(optarg);
                printf("mem_test total_bytes: %u\n", total_bytes);
                break;
            case 'p':
                access_bytes_per_pkt = atoi(optarg);
                printf("mem_test access_bytes_per_pkt: %u\n", access_bytes_per_pkt);
                break;
            case ':':  
                printf("option -%c needs a value\n", optopt);  
                break;  
            case '?':  
                printf("unknown option: -%c\n", optopt); 
                break; 
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
    
    pthread_t threads[4];
    int nf_idx[4] = {0, 1, 2, 3};
    for(int i = 0; i < num_nfs; i++){
        pthread_create(&threads[i], NULL, loop_func, (void*)(nf_idx+i));
    }
    for(int i = 0; i < num_nfs; i++){
        pthread_join(threads[i], NULL);
    }

    return 0;
}