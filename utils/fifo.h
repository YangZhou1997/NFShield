#ifndef __FIFO_H__
#define __FIFO_H__

#include <stdint.h>

#include "riscv_util.h"

// given each NF's MAX_UNACK_WINDOW is 512, while we have 4 NFs.
// 4098 pkt_bufs are enough
#define FIFO_SHIFT 12
#define FIFO_CAPACITY (1 << FIFO_SHIFT)
#define FIFO_CAPACITY_MASK (FIFO_CAPACITY - 1)

// the maximum number of items is FIFO_CAPACITY - 1
typedef struct {
  intptr_t pkts[FIFO_CAPACITY];
  int lens[FIFO_CAPACITY];
  uint32_t head_;
  uint32_t tail_;
  arch_spinlock_t spin_;
} fifo_t;

static inline void fifo_init(fifo_t *ff) {
  ff->head_ = 0;
  ff->tail_ = 0;
  ff->spin_.lock = 0;
}

static inline uint32_t load_acquire(uint32_t *p) {
  return __atomic_load_n(p, __ATOMIC_ACQUIRE);
}

static inline void store_release(uint32_t *p, uint32_t v) {
  __atomic_store_n(p, v, __ATOMIC_RELEASE);
}

static inline uint32_t fifo_size(fifo_t *ff) {
  uint32_t ret;
  uint32_t tail = load_acquire(&ff->tail_);
  uint32_t head = ff->head_;
  if (tail < head) {
    ret = tail + FIFO_CAPACITY - head;
  } else {
    ret = tail - head;
  }
  return ret;
}

static inline bool fifo_push(fifo_t *ff, intptr_t pkt_buf, int pkt_len) {
  arch_spin_lock(&ff->spin_);
  uint32_t tail = load_acquire(&ff->tail_);
  uint32_t new_tail = (tail + 1) & FIFO_CAPACITY_MASK;
  bool success = (new_tail != ff->head_);
  if (success) {
    ff->pkts[tail] = pkt_buf;
    ff->lens[tail] = pkt_len;
    store_release(&ff->tail_, new_tail);
  }
  arch_spin_unlock(&ff->spin_);
  return success;
}

static inline bool fifo_pop(fifo_t *ff, intptr_t *pkt_buf, int *pkt_len) {
  arch_spin_lock(&ff->spin_);
  uint32_t head = load_acquire(&ff->head_);
  uint32_t tail = ff->tail_;
  bool success = (head != tail);
  if (success) {
    *pkt_buf = ff->pkts[head];
    *pkt_len = ff->lens[head];
    store_release(&ff->head_, (head + 1) & FIFO_CAPACITY_MASK);
  }
  arch_spin_unlock(&ff->spin_);
  return success;
}

#endif  // __FIFO_H__