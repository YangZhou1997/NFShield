#ifndef __PKT_HEADER__
#define __PKT_HEADER__
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

#endif // __PKT_HEADER__