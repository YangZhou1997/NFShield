#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "mmio.h"
#include "encoding.h"
#include "icenic.h"
#include "common.h"

uint64_t out_packets[NPACKETS][PACKET_WORDS];
int total_req = 0, total_comp = 0;
char inflight[NPACKETS];

static void fill_packet(
	uint64_t *packet, uint64_t srcmac, uint64_t dstmac, int id)
{
    packet[0] = dstmac << 16;
	packet[1] = srcmac | (0x1008L << 48);
	packet[2] = id;
	
	for (int i = 3; i < PACKET_WORDS; i++)
		packet[i] = random();
}

static void process_loop(void)
{
	uint32_t counts, send_req, send_comp;
	static int req_id = 0, comp_id = 0;

	counts = nic_counts();
	send_req  = (counts >> NIC_COUNT_SEND_REQ)  & 0xff;
	send_comp = (counts >> NIC_COUNT_SEND_COMP) & 0xff;

	for (int i = 0; i < send_comp; i++) {
		nic_complete_send();
		inflight[comp_id] = 0;
		comp_id = (comp_id + 1) % NPACKETS;
		total_comp++;
	}

	for (int i = 0; i < send_req; i++) {
		if (inflight[req_id])
			break;
		nic_post_send_no_check((uint64_t) out_packets[req_id], PACKET_WORDS * 8);
		inflight[req_id] = 1;
		req_id = (req_id + 1) % NPACKETS;
		total_req++;
	}
}

static void finish_comp(void)
{
	int counts = nic_counts();
	int comp = (counts >> NIC_COUNT_SEND_COMP) & 0xff;

	for (int i = 0; i < comp; i++) {
		nic_complete_send();
		total_comp++;
	}
}

void thread_entry(int cid, int nc)
{
    if (cid != 0) {
        return;
    }
    // server: 1st node, 00:12:6d:00:00:02
    // client: 2nd node: 00:12:6d:00:00:03
	uint64_t srcmac = nic_macaddr();
	uint64_t dstmac = 0x0200006d1200;
    printf("srcmac = %lx, dstmac = %lx\n", srcmac, dstmac);
	
    uint64_t cycle;

	srandom(0xCFF32987);

	memset(inflight, 0, NPACKETS);
	for (int i = 0; i < NPACKETS; i++) {
		fill_packet(out_packets[i], srcmac, dstmac, i);
	}

	asm volatile ("fence");

    cycle = rdcycle();

	printf("Start bandwidth test @ %lu\n", cycle);

    uint64_t start_cycle = rdcycle();
	do {
        process_loop();
	    cycle = rdcycle() - start_cycle;
	} while (cycle < TEST_CYCLES);

	printf("cycles: %lu, completed: %d, mpps: %lf\n",
			cycle, total_comp, total_comp / (cycle/CPU_GHZ*1e-3));

	while (total_comp < total_req)
		finish_comp();

	return 0;
}