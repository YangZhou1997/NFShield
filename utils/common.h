#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// #include <pthread.h>
// #include <sched.h>

#define U32 0
#define U16 1
#define TUPLE 2
#define BOOL 3

#define PRINT_INTERVAL (10 * 1024)
// #define PRINT_INTERVAL (1)

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)
#define STRING(s) #s

#ifdef CAVIUM_PLATFORM
#include "cvmx-bootmem.h"
#include "cvmx.h"
#define zmalloc(total_size, ALIGN, table_name) \
  cvmx_bootmem_alloc_named(total_size, ALIGN, table_name)
#define zfree(block_name) cvmx_bootmem_free_named(block_name)
#else
#define zmalloc(total_size, ALIGN, table_name) malloc(total_size)
#define zfree(block_ptr) free(block_ptr)
#endif

typedef struct {
  uint32_t srcip;
  uint32_t dstip;
  uint16_t srcport;
  uint16_t dstport;
  uint8_t proto;
} five_tuple_t;

#define IS_EQUAL(key1, key2)                                           \
  ((key1.srcip == key2.srcip) && (key1.dstip == key2.dstip) &&         \
   (key1.srcport == key2.srcport) && (key1.dstport == key2.dstport) && \
   (key1.proto == key2.proto))

#define ALIGN 0x400  // 128 bytes
#define CAST_PTR(type, ptr) ((type)(ptr))

#define MAX_TABLE_NAME_LEN 32

#define CMS_SH1 17093
#define CMS_SH2 30169
#define CMS_SH3 52757
#define CMS_SH4 83233

// int set_affinity(uint32_t cpu_id){
//     cpu_set_t cpuset;
//     CPU_ZERO(&cpuset);
//     CPU_SET(cpu_id, &cpuset);
//     pthread_t thread = pthread_self();
//     return pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
// }

// inline void barrier(){
//     asm volatile("": : :"memory");
// }

// // only work on X86
// inline uint64_t rdtsc(){
// 	uint32_t a, d;
// 	asm volatile("rdtsc" : "=a" (a), "=d" (d));
// 	return ((uint64_t)a) | (((uint64_t)d) << 32);
// }

#define CPU_GHZ (3.2)
#define BUF_SIZ 1536
#define MAX_BATCH_SIZE 32

#define TEST_NPKTS (2 * 50 * 1024)
#define MAX_UNACK_WINDOW 512

#define DEFAULT_IF "eth0"
#define ETH_P_IP 0x0800  /* Internet Protocol packet	*/
#define ETH_P_ALL 0x0003 /* Every packet (be careful!!!) */
#define ETH_ALEN 6       /* Octets in one ethernet addr	 */

#define CUSTOM_PROTO_BASE 0x1234

// nftop-server : 1st node, 00:12:6d:00:00:02
// pktgen-client: 2nd node, 00:12:6d:00:00:03
uint64_t MAC_NFTOP = 0x0200006d1200;
uint64_t MAC_PKTGEN = 0x0300006d1200;

// the following is to facilitate parsing embedded file.h
typedef struct {
  const unsigned char* file_data;
  unsigned long long file_size;
  unsigned long long cur_read_pos;
} MY_FILE;

void init_my_fread(const unsigned char* file_data, unsigned long long file_size,
                   MY_FILE* file) {
  file->file_data = file_data;
  file->file_size = file_size;
  file->cur_read_pos = 0;
}
size_t my_fread(void* ptr, size_t size, size_t n, MY_FILE* file) {
  size_t read_size = size * n;
  if (read_size + file->cur_read_pos > file->file_size) {
    return 0;
  }
  memcpy(ptr, file->file_data + file->cur_read_pos, read_size);
  file->cur_read_pos += read_size;
  return read_size;
}

#endif /* __COMMON_H__ */
