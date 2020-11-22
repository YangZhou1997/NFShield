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

static char nf_names[7][128];
static int (*nf_init[7])();
static void (*nf_process[7])(uint8_t *);
static void (*nf_destroy[7])();

static int sockfd  = 0;
static struct sockaddr_ll send_sockaddr;
static struct ifreq if_mac;

void *loop_func(void *arg){
    int nf_idx = *(int*)arg;

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

    while(1){
        numpkts = recvfrom_batch(sockfd, BUF_SIZ, pkt_buf);
        if(numpkts <= 0){
            continue;
        }
        // printf("[loop_func %d] receiving numpkts %d\n", nf_idx, numpkts);
        for(int i = 0; i < numpkts; i++){
            nf_process[nf_idx](pkt_buf[i]->content);
            eh = (struct ether_hdr*) pkt_buf[i]->content;
        
            /* Ethernet header */
            // @yang, the nf mush swap the mac address;
            eh->d_addr.addr_bytes[0] = MY_DEST_MAC0;
        	eh->d_addr.addr_bytes[1] = MY_DEST_MAC1;
        	eh->d_addr.addr_bytes[2] = MY_DEST_MAC2;
        	eh->d_addr.addr_bytes[3] = MY_DEST_MAC3;
        	eh->d_addr.addr_bytes[4] = MY_DEST_MAC4;
        	eh->d_addr.addr_bytes[5] = eh->s_addr.addr_bytes[5];

            // @yang, you must set source mac correct in order to send it out. 
            eh->s_addr.addr_bytes[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
        	eh->s_addr.addr_bytes[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
        	eh->s_addr.addr_bytes[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
        	eh->s_addr.addr_bytes[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
        	eh->s_addr.addr_bytes[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
        	eh->s_addr.addr_bytes[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
        	
        	/* Ethertype field */
        	eh->ether_type = htons(ETH_P_IP);
        }
    
        sendto_batch(sockfd, numpkts, pkt_buf, &send_sockaddr);
    }
    nf_destroy[nf_idx]();
 
    for(int i = 0; i < MAX_BATCH_SIZE; i++){
        free(pkt_buf[i]->content);
        free(pkt_buf[i]);
    }
    return NULL;
}

int main(int argc, char* argv[]){
    int num_nfs = 0;


    nf_process[0] = l2_fwd;
    nf_process[1] = acl_fw;
    nf_process[2] = dpi;
    nf_process[3] = lpm;
    nf_process[4] = maglev;
    nf_process[5] = monitoring;
    nf_process[6] = nat_tcp_v4;

    nf_init[0] = l2_fwd_init;
    nf_init[1] = acl_fw_init;
    nf_init[2] = dpi_init;
    nf_init[3] = lpm_init;
    nf_init[4] = maglev_init;
    nf_init[5] = monitoring_init;
    nf_init[6] = nat_tcp_v4_init;

    nf_destroy[0] = l2_fwd_destroy;
    nf_destroy[1] = acl_fw_destroy;
    nf_destroy[2] = dpi_destroy;
    nf_destroy[3] = lpm_destroy;
    nf_destroy[4] = maglev_destroy;
    nf_destroy[5] = monitoring_destroy;
    nf_destroy[6] = nat_tcp_v4_destroy;

    int option;
    while((option = getopt(argc, argv, ":n:")) != -1){
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
                else{
                    printf("%s is not a valid nf name\n", optarg);
                    exit(0);
                }
                num_nfs++;
                break;
            case ':':  
                printf("option -%c needs a value\n", optopt);  
                break;  
            case '?':  
                printf("unknown option: -%c\n", optopt); 
                break; 
        }
    }
    printf("%d NFs: ", num_nfs);
    for(int i = 0; i < num_nfs; i++){
        printf("%s ", nf_names[i]);
    }
    printf("\n");
    
    init_socket(&sockfd, &send_sockaddr, &if_mac);

    pthread_t threads[7];
    int nf_idx[7] = {0, 1, 2, 3, 4, 5, 6};
    for(int i = 0; i < num_nfs; i++){
        pthread_create(&threads[i], NULL, loop_func, (void*)(nf_idx+i));
    }
    for(int i = 0; i < num_nfs; i++){
        pthread_join(threads[i], NULL);
    }

    return 0;
}