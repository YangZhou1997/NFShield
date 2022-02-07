#ifndef __NIC_LINUX_H__
#define __NIC_LINUX_H__

#define NIC_BASE 0x10016000L
#define NIC_SEND_REQ (NIC_BASE + 0)
#define NIC_RECV_REQ (NIC_BASE + 8)
#define NIC_SEND_COMP (NIC_BASE + 16)
#define NIC_RECV_COMP (NIC_BASE + 18)
#define NIC_COUNTS (NIC_BASE + 20)
#define NIC_MACADDR (NIC_BASE + 24)

#define NIC_COUNT_SEND_REQ 0
#define NIC_COUNT_RECV_REQ 8
#define NIC_COUNT_SEND_COMP 16
#define NIC_COUNT_RECV_COMP 24

#include <stdint.h>
#include "mmio_linux.h"
#include "virt_to_phys.h"

static inline void reg_write8(uintptr_t addr, uint8_t data)
{
    mmio_writec(&__mmio, addr - NIC_BASE, data);
}

static inline uint8_t reg_read8(uintptr_t addr)
{
    return mmio_readc(&__mmio, addr - NIC_BASE);
}

static inline void reg_write16(uintptr_t addr, uint16_t data)
{
    mmio_writes(&__mmio, addr - NIC_BASE, data);
}

static inline uint16_t reg_read16(uintptr_t addr)
{
    return mmio_reads(&__mmio, addr - NIC_BASE);
}

static inline void reg_write32(uintptr_t addr, uint32_t data)
{
    mmio_writel(&__mmio, addr - NIC_BASE, data);
}

static inline uint32_t reg_read32(uintptr_t addr)
{
    return mmio_readl(&__mmio, addr - NIC_BASE);
}

static inline void reg_write64(unsigned long addr, uint64_t data)
{
    mmio_writell(&__mmio, addr - NIC_BASE, data);
}

static inline uint64_t reg_read64(unsigned long addr)
{
    return mmio_readll(&__mmio, addr - NIC_BASE);
}

static inline uint32_t nic_counts(void)
{
	return reg_read32(NIC_COUNTS);
}

static inline void nic_post_send(uint64_t addr, uint64_t len)
{
	// uint64_t request = ((len & 0x7fff) << 48) | (addr & 0xffffffffffffL);
	// reg_write64(NIC_SEND_REQ, request);

	uintptr_t phy_addr = virt_to_phys_user(addr);
	uint64_t request = (len << 48) | (phy_addr & 0xffffffffffffL);
	reg_write64(NIC_SEND_REQ, request);
}

static inline void nic_complete_send(void)
{
	reg_read16(NIC_SEND_COMP);
}

static inline void nic_post_recv(uint64_t addr)
{
	// reg_write64(NIC_RECV_REQ, addr);
    
    uintptr_t phy_addr = virt_to_phys_user(addr);
	reg_write64(NIC_RECV_REQ, phy_addr);
}

static inline uint16_t nic_complete_recv(void)
{
	return reg_read16(NIC_RECV_COMP);
}

static inline uint64_t nic_macaddr(void)
{
	return reg_read64(NIC_MACADDR);
}

#endif // __NIC_LINUX_H__