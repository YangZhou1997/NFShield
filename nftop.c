#include "./nfs/acl-fw.h"
#include "./nfs/dpi.h"
#include "./nfs/lpm.h"
#include "./nfs/maglev.h"
#include "./nfs/mem-test.h"
#include "./nfs/monitoring.h"
#include "./nfs/nat-tcp-v4.h"
#include "./utils/common.h"
#include "./utils/encoding.h"
#include "./utils/icenic_wrapper.h"
#include "./utils/pkt-header.h"
#include "./utils/pkt-ops.h"
#include "config.h"

int l2_fwd_init() { return 0; }
void l2_fwd(uint8_t *pkt_ptr) {
  swap_mac_addr(pkt_ptr);
  return;
}
void l2_fwd_destroy() { return; }

static char *nf_names[8];
static int (*nf_init[8])();
static void (*nf_process[8])(uint8_t *);
static void (*nf_destroy[8])();

#define WARMUP_NPKTS 10000
#define TEST_NPKTS 20000
#define PRINT_INTERVAL 10000

#define NCORES 4
static int num_nfs = 0;

barrier_t nic_boot_pkt_barrier;

void *loop_func(int nf_idx) {
  if (nf_init[nf_idx]() < 0) {
    printf("nf_init error, exit\n");
    exit(0);
  }

  nic_boot_pkt(nf_idx);
  barrier_wait(&nic_boot_pkt_barrier);
  printf("%d loop_func after barrier_wait\n", nf_idx);

  // debugging purpose
  if (nf_idx != 0) {
    return NULL;
  }

  uint64_t start = rdcycle();
  uint64_t pkt_size_sum = 0;
  uint32_t pkt_num = 0;
  pkt_t cur_pkt;
  while (1) {
    int numpkts = recvfrom_single(nf_idx, BUF_SIZ, &cur_pkt);
    if (numpkts <= 0) {
      continue;
    }
    // printf("[loop_func %d] receiving numpkts %d\n", nf_idx, numpkts);
    for (int i = 0; i < numpkts; i++) {
      nf_process[nf_idx](cur_pkt.content + NET_IP_ALIGN);

      pkt_size_sum += cur_pkt.len;
      pkt_num++;

      if (pkt_num % PRINT_INTERVAL == 0) {
        printf("%s (nf_idx %u): pkts received %u, avg_pkt_size %lu\n",
               nf_names[nf_idx], nf_idx, pkt_num, pkt_size_sum / pkt_num);
      }
      if (pkt_num >= TEST_NPKTS + WARMUP_NPKTS) {
        double time_taken = (rdcycle() - start) / CPU_GHZ * 1e-3;
        printf(
            "%s (nf_idx %u): processed pkts %u, elapsed time %lu us, "
            "processing rate %lu Kpps\n",
            nf_names[nf_idx], nf_idx, TEST_NPKTS + WARMUP_NPKTS,
            (uint64_t)time_taken,
            (uint64_t)((TEST_NPKTS + WARMUP_NPKTS) / time_taken * 1e3));
        // stopping the pktgen.
        struct tcp_hdr *tcph =
            (struct tcp_hdr *)(cur_pkt.content + ETH_HEADER_SIZE +
                               IP_HEADER_SIZE);
        tcph->recv_ack = 0xFFFFFFFF;
        goto finished;
      }
    }
    sendto_single(nf_idx, &cur_pkt);
  }
finished:
  sendto_single(nf_idx, &cur_pkt);
  nf_destroy[nf_idx]();
  return NULL;
}

// 1.5GB memory for malloc
#define MALLOC_SIZE (1536 * 1024 * 1024)
static uint8_t malloc_bytes[MALLOC_SIZE];

arch_spinlock_t init_lock;
int if_inited = 0;

void init_nfs_once() {
  arch_spin_lock(&init_lock);
  if (if_inited) {
    arch_spin_unlock(&init_lock);
    return;
  }
  if_inited = 1;

  barrier_init(NCORES, &nic_boot_pkt_barrier);
  malloc_addblock(malloc_bytes, MALLOC_SIZE);

  char *nf_names_all[8] = {"l2_fwd", "acl_fw",     "dpi",        "lpm",
                           "maglev", "monitoring", "nat_tcp_v4", "mem_test"};
  int (*nf_init_all[8])() = {l2_fwd_init,     acl_fw_init,  dpi_init,
                             lpm_init,        maglev_init,  monitoring_init,
                             nat_tcp_v4_init, mem_test_init};
  void (*nf_process_all[8])(uint8_t *) = {
      l2_fwd, acl_fw, dpi, lpm, maglev, monitoring, nat_tcp_v4, mem_test};
  void (*nf_destroy_all[8])() = {
      l2_fwd_destroy, acl_fw_destroy,     dpi_destroy,        lpm_destroy,
      maglev_destroy, monitoring_destroy, nat_tcp_v4_destroy, mem_test_destroy};

  printf("NF_STRINGS: %s\n", NF_STRINGS);
  char buf[512];
  strcpy(buf, NF_STRINGS);

  char *token;
  for (token = strtok(buf, ":"); token; token = strtok(NULL, ":")) {
    printf("token: %s\n", token);
    for (int i = 0; i < 8; i++) {
      if (!strcmp(token, nf_names_all[i])) {
        printf("valid NF #%d: %s\n", num_nfs, token);
        nf_process[num_nfs] = nf_process_all[i];
        nf_init[num_nfs] = nf_init_all[i];
        nf_destroy[num_nfs] = nf_destroy_all[i];
        nf_names[num_nfs] = nf_names_all[i];
        num_nfs++;
        break;
      }
    }
  }

  if (num_nfs != 4) {
    printf("Only support num_nfs == 4!\n");
    exit(0);
  }

  printf("%d NFs: ", num_nfs);
  for (int i = 0; i < num_nfs; i++) {
    printf("%s ", nf_names[i]);
  }
  printf("\n");
  arch_spin_unlock(&init_lock);
}

void thread_entry(int cid, int nc) {
  init_nfs_once();
  loop_func(cid);
}