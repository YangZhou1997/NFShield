#ifndef __RAW_SOCKET_
#define __RAW_SOCKET_


#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <signal.h>
#include <sys/time.h> 
#include <unistd.h>
#include "pkt-header.h"

#define DEFAULT_IF	"eth0"
#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ETH_P_ALL	0x0003		/* Every packet (be careful!!!) */
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */

#define BUF_SIZ		1536
#define MAX_BATCH_SIZE 32

#define WARMUP_NPKTS (1*50*1024)
#define TEST_NPKTS (2*50*1024)
#define MAX_UNACK_WINDOW 512

#define CUSTOM_PROTO_BASE 0x1234
// #define CUSTOM_PROTO_BASE 0

void init_socket(int *sockfd_p, struct sockaddr_ll* send_sockaddr, struct ifreq *if_mac_p, uint8_t dstmac[], int nf_idx){
    // setup socket for receiving packets.
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
	// if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) == -1) {
	// if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
    // you cannot use the occupied protocol number. 
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons((uint16_t)(nf_idx + CUSTOM_PROTO_BASE)))) == -1) {
        printf("[recv_pacekts] socket");
		return;
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	// strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	// ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	// ifopts.ifr_flags |= IFF_PROMISC;
	// ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
    
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		printf("[recv_pacekts] setsockopt");
		close(sockfd);
        return;
	}
	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
		printf("[recv_pacekts] SO_BINDTODEVICE");
		close(sockfd);
        return;
	}
    // uint32_t bufsize = 1024 * 200; // 200KB r/w buffer size for each sockets. 
    // if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
	// 	printf("[recv_pacekts] SO_BINDTODEVICE");
    // }
    // if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
	// 	printf("[recv_pacekts] SO_BINDTODEVICE");
    // }

    // setup socket for sending packets. 
    struct ifreq if_idx;
	struct ifreq if_mac;
	struct sockaddr_ll socket_address;
    
	/* Open RAW socket to send on */
	// if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	//     printf("[send_pacekts %d] socket", nf_idx);
    //     return;
	// }

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0){
	    printf("[send_pacekts] SIOCGIFINDEX");
		close(sockfd);
        return;
    }
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0){
	    printf("[send_pacekts] SIOCGIFHWADDR");
		close(sockfd);
        return;
    }

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = dstmac[0];
	socket_address.sll_addr[1] = dstmac[1];
	socket_address.sll_addr[2] = dstmac[2];
	socket_address.sll_addr[3] = dstmac[3];
	socket_address.sll_addr[4] = dstmac[4];
	socket_address.sll_addr[5] = dstmac[5];
    
    *sockfd_p = sockfd;
    *send_sockaddr = socket_address;
    *if_mac_p = if_mac;

    return;
}

int recvfrom_batch(int sockfd, int buff_size, pkt_t** pkt_buf){
    int cnt = 0;
    int numbytes = 0;
    while(cnt < MAX_BATCH_SIZE){
        // once returned, this is guaranteed to return a full ethernet frame. 
        numbytes = recvfrom(sockfd, pkt_buf[cnt]->content, buff_size, MSG_DONTWAIT, NULL, NULL);
        if(numbytes <= 0){
            return cnt;
        }
        pkt_buf[cnt]->len = numbytes;
        cnt ++;
    }
    return cnt;
}

int recvfrom_single(int sockfd, int buff_size, pkt_t* pkt_buf){
    int numbytes = 0;
    // once returned, this is guaranteed to return a full ethernet frame. 
    numbytes = recvfrom(sockfd, pkt_buf->content, buff_size, MSG_DONTWAIT, NULL, NULL);
    if(numbytes <= 0){
        return 0;
    }
    pkt_buf->len = numbytes;
    return 1;
}

int sendto_batch(int sockfd, int batch_size, pkt_t** pkt_buf, struct sockaddr_ll* send_sockaddr){
    int cnt = 0;
    while(cnt < batch_size){
        uint32_t bytes, sent = 0;
        do{
            bytes = sendto(sockfd, pkt_buf[cnt]->content + sent, pkt_buf[cnt]->len - sent, MSG_DONTWAIT, (struct sockaddr*)send_sockaddr, sizeof(struct sockaddr_ll));
            if(bytes < 0){
                continue;
            }
            sent += bytes;
        }while(sent != pkt_buf[cnt]->len);
        cnt ++;
    }
    return cnt;
}

int sendto_single(int sockfd, pkt_t* pkt_buf, struct sockaddr_ll* send_sockaddr){
    uint32_t bytes, sent = 0;
    do{
        bytes = sendto(sockfd, pkt_buf->content + sent, pkt_buf->len - sent, MSG_DONTWAIT, (struct sockaddr*)send_sockaddr, sizeof(struct sockaddr_ll));
        if(bytes < 0){
            continue;
        }
        sent += bytes;
    }while(sent != pkt_buf->len);
    return 1;
}
#endif