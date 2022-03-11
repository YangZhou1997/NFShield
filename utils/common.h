#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef	__cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif
#include <stdint.h>
#include <pthread.h>
#include <sched.h>

#define U32 0
#define U16 1
#define TUPLE 2
#define BOOL 3

#define PRINT_INTERVAL (10 * 1024)
// #define PRINT_INTERVAL (1)

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

#ifdef CAVIUM_PLATFORM
#include "cvmx.h"
#include "cvmx-bootmem.h"
#define zmalloc(total_size, ALIGN, table_name) \
    cvmx_bootmem_alloc_named(total_size, ALIGN, table_name)
#define zfree(block_name) \
    cvmx_bootmem_free_named(block_name)
#else
#define zmalloc(total_size, ALIGN, table_name) \
    malloc(total_size)
#define zfree(block_ptr) \
    free(block_ptr)
#endif

typedef struct
{
	uint32_t srcip;
	uint32_t dstip;
	uint16_t srcport;
	uint16_t dstport;
	uint8_t proto;
}five_tuple_t;

#define IS_EQUAL(key1, key2) ((key1.srcip == key2.srcip) && (key1.dstip == key2.dstip) && (key1.srcport == key2.srcport) && (key1.dstport == key2.dstport) && (key1.proto == key2.proto))

#define ALIGN 0x400 // 128 bytes       
#define CAST_PTR(type, ptr) ((type)(ptr))

#define MAX_TABLE_NAME_LEN 32

#define CMS_SH1 17093
#define CMS_SH2 30169
#define CMS_SH3 52757
#define CMS_SH4 83233

int set_affinity(uint32_t cpu_id){
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_t thread = pthread_self();
    return pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
}

inline void barrier(){
    asm volatile("": : :"memory");
}

// only work on X86
inline uint64_t rdtsc(){
	uint32_t a, d;
	asm volatile("rdtsc" : "=a" (a), "=d" (d));
	return ((uint64_t)a) | (((uint64_t)d) << 32);
}

#ifdef	__cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif /* __COMMON_H__ */
