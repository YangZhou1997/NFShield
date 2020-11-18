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
#include "./utils/common.h"
#include "./utils/lpm-algo.h"
#include "./utils/pkt-puller.h"

#define MY_DEST_MAC0	0x00
#define MY_DEST_MAC1	0x00
#define MY_DEST_MAC2	0x00
#define MY_DEST_MAC3	0x00
#define MY_DEST_MAC4	0x00
#define MY_DEST_MAC5	0x01

#define DEFAULT_IF	"eth0"
#define BUF_SIZ		1536
#define ETHER_TYPE	0x0800
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define MIN(x, y) (x < y ? x : y)

#define MAX_UNACK_WINDOW 128
static volatile uint64_t unack_packets = 0;
static volatile uint64_t sent_pkts = 0;
static volatile uint64_t received_pkts = 0;

static volatile uint8_t force_quit_send;
static volatile uint8_t force_quit_recv;

#define TEST_NPKTS (2*10*1024)
#define WARMUP_NPKTS (1*10*1024)

void *send_pkt_func(void *arg) 
{
	struct ifreq if_idx;
	struct ifreq if_mac;
	char ifName[IFNAMSIZ];
	struct sockaddr_ll socket_address;
    int sockfd;

	strcpy(ifName, DEFAULT_IF);

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	    perror("[send_pacekts] socket");
        return NULL;
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0){
	    perror("[send_pacekts] SIOCGIFINDEX");
		close(sockfd);
        return NULL;
    }
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0){
	    perror("[send_pacekts] SIOCGIFHWADDR");
		close(sockfd);
        return NULL;
    }

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;


    // TODO: pkt_puller load all necessary packets. 
    load_pkt((char *)arg);

    for(int i = 0; i < PKT_NUM; i++){
        struct ether_hdr* eh = (struct ether_hdr*) pkts[i].content;
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
    	eh->ether_type = htons(ETHER_TYPE);
    }

    struct timeval start, end; 
    uint64_t burst_size;
    struct tcp_hdr *tcph;
    while(sent_pkts < TEST_NPKTS + WARMUP_NPKTS){
        while(sent_pkts >= received_pkts && (unack_packets = sent_pkts - received_pkts) >= MAX_UNACK_WINDOW){
            if(force_quit_send)
				break;
        }
        burst_size = MAX_UNACK_WINDOW - unack_packets;
        burst_size = MIN(burst_size, TEST_NPKTS + WARMUP_NPKTS - sent_pkts);

        for(int i = 0; i < burst_size; i++){
            pkt_t *raw_pkt = next_pkt();
        	tcph = (struct tcp_hdr *) (raw_pkt->content + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));
            tcph->sent_seq = 0xdeadbeef;
            tcph->recv_ack = sent_pkts + i;

        	/* Send packet */
        	if (sendto(sockfd, raw_pkt->content, raw_pkt->len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0){
        	    printf("[send_pacekts] send failed\n");
            }
            else{
                sent_pkts ++;
            }
 
            if(sent_pkts == WARMUP_NPKTS){
                gettimeofday(&start, NULL); 
            }
            if(sent_pkts % PRINT_INTERVAL == 0){
                printf("[send_pacekts] number of pkts sent: %u\n", sent_pkts);
            }
        }
       if(force_quit_send)
			break;
    }
    gettimeofday(&end, NULL); 
    sent_pkts -= WARMUP_NPKTS;
    double time_taken; 
    time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    printf("[send_pacekts] %llu pkt sent, %.4lf Mpps\n", sent_pkts, (double)(sent_pkts) * 1e-6 / time_taken);
    
    // ending packet receiving. 
    force_quit_recv = 1;

    return NULL;
}

void * recv_pkt_func(void *arg){
	char sender[INET6_ADDRSTRLEN];
	int sockfd, ret, i;
	int sockopt;
	ssize_t numbytes;
	struct ifreq ifopts;	/* set promiscuous mode */
	struct ifreq if_ip;	/* get ip addr */
	struct sockaddr_storage their_addr;
	uint8_t buf[BUF_SIZ];
	char ifName[IFNAMSIZ];
	
	/* Get interface name */
	strcpy(ifName, DEFAULT_IF);

	/* Header structures */
	struct ether_hdr *eh = (struct ether_hdr *) buf;
	struct ipv4_hdr *iph = (struct ipv4_hdr *) (buf + sizeof(struct ether_hdr));
	struct tcp_hdr *tcph = (struct tcp_hdr *) (buf + sizeof(struct ipv4_hdr) + sizeof(struct ether_hdr));

	memset(&if_ip, 0, sizeof(struct ifreq));

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
		perror("[recv_pacekts] socket");	
		return NULL;
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("[recv_pacekts] setsockopt");
		close(sockfd);
        return NULL;
	}
	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
		perror("[recv_pacekts] SO_BINDTODEVICE");
		close(sockfd);
        return NULL;
	}


    uint64_t invalid_pkts = 0, lost_pkt_during_cold_start = 0;
    uint8_t warmup_end = 0;
	while(force_quit_send != 0){
        numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
    	// printf("listener: got packet %lu bytes\n", numbytes);
        if(tcph->sent_seq != 0xdeadbeef){
            invalid_pkts ++;
            continue;
        }
        else{
            received_pkts ++;
        }
        uint32_t pkt_idx = tcph->recv_ack;
        if(!warmup_end && pkt_idx >= WARMUP_NPKTS - 1)
        {
    		warmup_end = 1;
            // lost_pkt_during_cold_start = npkt * WARMUP_THRES - 1 - (received_pkts + i); 
            lost_pkt_during_cold_start = pkt_idx - received_pkts; 
            printf("[recv_pacekts] warm up ends, lost_pkt_during_cold_start = %llu\n", lost_pkt_during_cold_start);
    		printf("[recv_pacekts] pkt_idx %u, received_pkts %llu\n", pkt_idx, received_pkts);
    		fflush(stdout);
        }
        if(received_pkts % PRINT_INTERVAL == 0){
            printf("[recv_pacekts] number of pkts received: %u\n", received_pkts);
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

int main(){
    signal(SIGINT, signal_handler);
    force_quit_recv = 0;
    force_quit_send = 0;

    char * filename = "./data/ictf2010_100kflow.dat";
    pthread_t send_thread;
    pthread_t recv_thread;
    pthread_create(&send_thread, NULL, send_pkt_func, (void *)filename);
    pthread_create(&recv_thread, NULL, recv_pkt_func, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    return 0;
}
    