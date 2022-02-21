#ifndef __RAW_ICENET_
#define __RAW_ICENET_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pkt-header.h"
#include "common.h"
#include "icenic.h"

// nftop-server : 1st node, 00:12:6d:00:00:02
// pktgen-client: 2nd node, 00:12:6d:00:00:03
uint64_t MAC_NFTOP = 0x0200006d1200;
uint64_t MAC_PKTGEN = 0x0300006d1200;

int recvfrom_batch(int core_id, int buff_size, pkt_t* pkt_buf){
    int cnt = 0;
    int numbytes = 0;
    while(cnt < MAX_BATCH_SIZE){
        if (nic_recv_req_avail() == 0) {
            return cnt;
        }
        nic_post_recv_no_check((uintptr_t)pkt_buf[cnt].content);
        pkt_buf[cnt].len = nic_wait_recv();
        cnt ++;
    }
    return cnt;
}

int recvfrom_single(int core_id, int buff_size, pkt_t* pkt_buf){
    if (nic_recv_req_avail() == 0) {
        return 0;
    }
    nic_post_recv_no_check((uintptr_t)pkt_buf);
    pkt_buf->len = nic_wait_recv();
    return 1;
}

int sendto_batch(int core_id, int batch_size, pkt_t* pkt_buf){
    int cnt = 0;
    while(cnt < batch_size){
        nic_post_send(pkt_buf[cnt].content, pkt_buf[cnt].len);
        cnt ++;
    }
    nic_wait_send_batch(cnt);
    return cnt;
}

int sendto_single(int core_id, pkt_t* pkt_buf){
    nic_send(pkt_buf->content, pkt_buf->len);
    return 1;
}
#endif