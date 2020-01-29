#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef	__cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

// enum value_type{U32, U16, TUPLE, BOOL};
#include <stdint.h>

#define U32 0
#define U16 1
#define TUPLE 2
#define BOOL 3


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

#ifdef	__cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif /* __COMMON_H__ */
