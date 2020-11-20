#include "./utils/common.h"
#include "./utils/pkt-ops.h"
#include "./utils/pkt-header.h"
#include "./utils/raw_socket.h"

void l2_fwd_init(){
    return;
}
void l2_fwd(uint8_t *pkt_ptr){
    swap_mac_addr(pkt_ptr);
    return;
}
void l2_fwd_destroy(){
    return;
}

static void (*nfs[4])(uint8_t *);
static void (*nf_init[4])();
static void (*nf_destroy[4])();

static int sockfd  = 0;
static struct sockaddr_ll send_sockaddr;
static struct ifreq if_mac;

void *loop_func(void *arg){
    int nf_idx = *(int*)arg;
	uint8_t buf[BUF_SIZ];
    int numbytes = 0;
    struct ether_hdr* eh;

    while(1){
        numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
        // printf("[loop_func %d] receiving numbytes %d\n", nf_idx, numbytes);
        // calling nf processing function. 
        nfs[nf_idx](buf);
    
        eh = (struct ether_hdr*) buf;
        /* Ethernet header */
        // the nf mush swap the mac address;
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
    
    	while (sendto(sockfd, buf, numbytes, 0, (struct sockaddr*)&send_sockaddr, sizeof(struct sockaddr_ll)) < 0){
            printf("[loop_func %d] sendto errors\n", nf_idx);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]){
    int num_nfs = 1;

    int option;
    while((option = getopt(argc, argv, ":n:")) != -1){
        switch (option) {
            case 'n':
                num_nfs = atoi(optarg);
                printf("number of NFs: %d\n", num_nfs);
                break;
            case ':':  
                printf("option -%c needs a value\n", optopt);  
                break;  
            case '?':  
                printf("unknown option: -%c\n", optopt); 
                break; 
        }
    }

#define TEST

#ifdef TEST
    nfs[0] = l2_fwd;
    nfs[1] = l2_fwd;
    nfs[2] = l2_fwd;
    nfs[3] = l2_fwd;
#else
    nfs[0] = l2_fwd;
    nfs[1] = l2_fwd;
    nfs[2] = l2_fwd;
    nfs[3] = l2_fwd;
#endif // TEST

    init_socket(&sockfd, &send_sockaddr, &if_mac);

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