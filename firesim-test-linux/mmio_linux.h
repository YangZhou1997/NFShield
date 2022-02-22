#ifndef __MMIO_LINUX_H__
#define __MMIO_LINUX_H__

/*
 * Copyright (C) 2012 Žilvinas Valinskas
 * See LICENSE for more information.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct mmio {
  unsigned long iobase; /* getpagesize() aligned, see mmap(2) */
  unsigned long offset; /* additional offset from iobase */
  unsigned long range;  /* N * uint32_t read/write ops. */

  void *iomem;
  size_t iosize;
  int kmem; /* 0 - /dev/mem, 1 - /dev/kmem */
};

struct mmio __mmio;

static inline uint8_t readc(void *ptr) {
  uint8_t *data = (uint8_t *)ptr;
  return *data;
}

static inline void writec(uint8_t value, void *ptr) {
  uint8_t *data = (uint8_t *)ptr;
  *data = value;
}

uint8_t mmio_readc(const struct mmio *io, unsigned int offset) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  return readc(addr);
}

void mmio_writec(const struct mmio *io, unsigned int offset, uint8_t value) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  writec(value, addr);
}

static inline uint16_t reads(void *ptr) {
  uint16_t *data = (uint16_t *)ptr;
  return *data;
}

static inline void writes(uint16_t value, void *ptr) {
  uint16_t *data = (uint16_t *)ptr;
  *data = value;
}

uint16_t mmio_reads(const struct mmio *io, unsigned int offset) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  return reads(addr);
}

void mmio_writes(const struct mmio *io, unsigned int offset, uint16_t value) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  writes(value, addr);
}

static inline uint32_t readl(void *ptr) {
  uint32_t *data = (uint32_t *)ptr;
  return *data;
}

static inline void writel(uint32_t value, void *ptr) {
  uint32_t *data = (uint32_t *)ptr;
  *data = value;
}

uint32_t mmio_readl(const struct mmio *io, unsigned int offset) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  return readl(addr);
}

void mmio_writel(const struct mmio *io, unsigned int offset, uint32_t value) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  writel(value, addr);
}

static inline uint64_t readll(void *ptr) {
  uint64_t *data = (uint64_t *)ptr;
  return *data;
}

static inline void writell(uint64_t value, void *ptr) {
  uint64_t *data = (uint64_t *)ptr;
  *data = value;
}

uint64_t mmio_readll(const struct mmio *io, unsigned int offset) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  return readll(addr);
}

void mmio_writell(const struct mmio *io, unsigned int offset, uint64_t value) {
  void *addr;

  addr = io->iomem + io->offset + offset;
  writell(value, addr);
}

static void mmio_normalize(struct mmio *mo) {
  int npages = 0;

  mo->iobase += mo->offset;
  mo->offset = mo->iobase & (getpagesize() - 1);
  mo->iobase = mo->iobase & ~(getpagesize() - 1);

  npages += (mo->range * sizeof(uint32_t)) / getpagesize();
  npages += 1;
  mo->iosize = npages * getpagesize();
}

static void mmio_init(struct mmio *mo) {
  char *device;
  int iofd;

  if (!mo->kmem)
    device = "/dev/mem";
  else
    device = "/dev/kmem";

  iofd = open(device, O_RDWR);
  if (iofd < 0) printf("open() failed");

  mo->iomem = mmap(NULL, mo->iosize, PROT_READ | PROT_WRITE, MAP_SHARED, iofd,
                   mo->iobase);

  if (mo->iomem == MAP_FAILED) printf("can't map @ %0lX", mo->iobase);

  close(iofd);
}

int mmio_map(struct mmio *io, unsigned long base, size_t length) {
  memset(io, 0, sizeof(*io));

  io->iobase = base;
  io->offset = 0;
  io->range = length;

  mmio_normalize(io);
  mmio_init(io);

  return 0;
}

void mmio_unmap(struct mmio *io) {
  if (munmap(io->iomem, io->iosize)) printf("can't unmap @ %lX", io->iobase);

  memset(io, 0, sizeof(*io));
}

#endif  // __MMIO_LINUX_H__