#ifndef __CONFIG_H__
#define __CONFIG_H__

#define NCORES 4

#ifndef NF_STRINGS
#define NF_STRINGS "l2_fwd:l2_fwd:l2_fwd:l2_fwd"
#endif

#ifndef MEM_TEST_TOTAL_BYTES
#define MEM_TEST_TOTAL_BYTES 2097152
#endif

#ifndef MEM_TEST_ACCESS_BYTES_PER_PKT
#define MEM_TEST_ACCESS_BYTES_PER_PKT 1024
#endif

#endif  // __CONFIG_H__