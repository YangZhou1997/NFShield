#ifndef _ETHERNET_H
#define _ETHERNET_H

#include <stdint.h>

#define ETHER_ADDR_LEN  6 /**< Length of Ethernet address. */
#define ETHER_TYPE_LEN  2 /**< Length of Ethernet type field. */

struct ether_addr {
	uint8_t addr_bytes[ETHER_ADDR_LEN]; /**< Addr bytes in tx order */
} __attribute__((__packed__));

struct ether_hdr {
	struct ether_addr d_addr; /**< Destination address. */
	struct ether_addr s_addr; /**< Source address. */
	uint16_t ether_type;      /**< Frame type. */
} __attribute__((__packed__));

#endif // _ETHERNET_H

#ifndef _IP_H
#define _IP_H

#include <stdint.h>

struct ipv4_hdr {
	uint8_t  version_ihl;		/**< version and header length */
	uint8_t  type_of_service;	/**< type of service */
	uint16_t total_length;		/**< length of packet */
	uint16_t packet_id;		/**< packet ID */
	uint16_t fragment_offset;	/**< fragmentation offset */
	uint8_t  time_to_live;		/**< time to live */
	uint8_t  next_proto_id;		/**< protocol ID */
	uint16_t hdr_checksum;		/**< header checksum */
	uint32_t src_addr;		/**< source address */
	uint32_t dst_addr;		/**< destination address */
} __attribute__((__packed__));

#endif // _IP_H

#ifndef _TCP_H
#define _TCP_H

#include <stdint.h>

struct tcp_hdr {
	uint16_t src_port;  /**< TCP source port. */
	uint16_t dst_port;  /**< TCP destination port. */
	uint32_t sent_seq;  /**< TX data sequence number. */
	uint32_t recv_ack;  /**< RX data acknowledgement sequence number. */
	uint8_t  data_off;  /**< Data offset. */
	uint8_t  tcp_flags; /**< TCP flags */
	uint16_t rx_win;    /**< RX flow control window. */
	uint16_t cksum;     /**< TCP checksum. */
	uint16_t tcp_urp;   /**< TCP urgent pointer, if any. */
} __attribute__((__packed__));

#endif // _TCP_H

#ifndef _PCAP_H
#define _PCAP_H

#include <stdint.h>

struct pcap_hdr {
        uint32_t magic_number;   /* magic number */
        uint16_t version_major;  /* major version number */
        uint16_t version_minor;  /* minor version number */
        int32_t  thiszone;       /* GMT to local correction */
        uint32_t sigfigs;        /* accuracy of timestamps */
        uint32_t snaplen;        /* max length of captured packets, in octets */
        uint32_t network;        /* data link type */
};

struct pcaprec_hdr {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
};

#endif // _PCAP_H

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef	__cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

// enum value_type{U32, U16, TUPLE, BOOL};

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
