/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/time.h> 
#include "./utils/pkt-puller3.h" // zipf with preloaded seq, a bit faster than the first; modest memory (100k distinct packets, and 2m seq number)
#include "./utils/raw_socket.h"
#include "./utils/parsing_mac.h"

#define DEFAULT_IF	"eth0"

int main(int argc, char *argv[])
{
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];
    uint8_t dstmac[6];
    memset(dstmac, 0, 6);
	
	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);
    if(argc > 2){
        parse_mac(dstmac, argv[2]);
    }
    printf("my dest mac address: %02x:%02x:%02x:%02x:%02x:%02x\n", dstmac[0], dstmac[1], dstmac[2], dstmac[3], dstmac[4], dstmac[5]);

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	    perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");


    load_pkt("./data/ictf2010_100kflow.dat");
    for(int i = 0; i < PKT_NUM; i++){
        struct ether_hdr* eh = (struct ether_hdr*) pkts[i].content;
        /* Ethernet header */
        eh->s_addr.addr_bytes[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
    	eh->s_addr.addr_bytes[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
    	eh->s_addr.addr_bytes[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
    	eh->s_addr.addr_bytes[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
    	eh->s_addr.addr_bytes[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
    	eh->s_addr.addr_bytes[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
    	eh->d_addr.addr_bytes[0] = dstmac[0];
    	eh->d_addr.addr_bytes[1] = dstmac[1];
    	eh->d_addr.addr_bytes[2] = dstmac[2];
    	eh->d_addr.addr_bytes[3] = dstmac[3];
    	eh->d_addr.addr_bytes[4] = dstmac[4];
    	eh->d_addr.addr_bytes[5] = dstmac[5];
    	/* Ethertype field */
    	eh->ether_type = htons(ETH_P_IP);
    }
    pkt_t* pkt_buf = NULL;

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

    struct timeval start, end; 
    gettimeofday(&start, NULL);
	/* Send packet */
    for(int i = 0; i <= 102400; i++){
        pkt_buf = next_pkt(0);

        sendto(sockfd, pkt_buf->content, pkt_buf->len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
        if(i % 10240 == 0){
            printf("sending packet %d\n", i);
        }
    }
    gettimeofday(&end, NULL); 
    double time_taken; 
    time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    printf("%llu pkt sent, %.8lf Mpps\n", 102400, 102400 * 1e-6 / time_taken);
    
	return 0;
}