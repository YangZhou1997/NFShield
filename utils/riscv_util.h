// See LICENSE for license details.

#ifndef __UTIL_H
#define __UTIL_H

//--------------------------------------------------------------------------
// Macros

// Set HOST_DEBUG to 1 if you are going to compile this for a host
// machine (ie Athena/Linux) for debug purposes and set HOST_DEBUG
// to 0 if you are compiling with the smips-gcc toolchain.

#ifndef HOST_DEBUG
#define HOST_DEBUG 0
#endif

// Set PREALLOCATE to 1 if you want to preallocate the benchmark
// function before starting stats. If you have instruction/data
// caches and you don't want to count the overhead of misses, then
// you will need to use preallocation.

#ifndef PREALLOCATE
#define PREALLOCATE 0
#endif

// Set SET_STATS to 1 if you want to carve out the piece that actually
// does the computation.

#if HOST_DEBUG
#include <stdio.h>
static void setStats(int enable) {}
#else
extern void setStats(int enable);
#endif

#include <stdint.h>

#define static_assert(cond) \
  switch (0) {              \
    case 0:                 \
    case !!(long)(cond):;   \
  }

static void __attribute__((noinline)) barrier(int ncores) {
  static volatile int sense;
  static volatile int count;
  static __thread int threadsense;

  __sync_synchronize();

  threadsense = !threadsense;
  if (__sync_fetch_and_add(&count, 1) == ncores - 1) {
    count = 0;
    sense = threadsense;
  } else
    while (sense != threadsense)
      ;

  __sync_synchronize();
}

#ifdef __riscv
#include "encoding.h"
#endif

#define stringify_1(s) #s
#define stringify(s) stringify_1(s)
#define stats(code, iter)                                                  \
  do {                                                                     \
    unsigned long _c = -read_csr(mcycle), _i = -read_csr(minstret);        \
    code;                                                                  \
    _c += read_csr(mcycle), _i += read_csr(minstret);                      \
    if (cid == 0)                                                          \
      printf("\n%s: %ld cycles, %ld.%ld cycles/iter, %ld.%ld CPI\n",       \
             stringify(code), _c, _c / iter, 10 * _c / iter % 10, _c / _i, \
             10 * _c / _i % 10);                                           \
  } while (0)

typedef struct {
  volatile unsigned int lock;
} arch_spinlock_t;

#define arch_spin_is_locked(x) ((x)->lock != 0)

static inline void arch_spin_unlock(arch_spinlock_t* lock) {
  asm volatile("amoswap.w.rl x0, x0, %0" : "=A"(lock->lock)::"memory");
}

static inline int arch_spin_trylock(arch_spinlock_t* lock) {
  int tmp = 1, busy;
  asm volatile("amoswap.w.aq %0, %2, %1"
               : "=r"(busy), "+A"(lock->lock)
               : "r"(tmp)
               : "memory");
  return !busy;
}

static inline void arch_spin_lock(arch_spinlock_t* lock) {
  while (1) {
    if (arch_spin_is_locked(lock)) {
      continue;
    }
    if (arch_spin_trylock(lock)) {
      break;
    }
  }
}

typedef struct {
  volatile unsigned int countdown;
  arch_spinlock_t lock;
} barrier_t;

// called once, init a global barrier.
static inline void barrier_init(int countdown_value, barrier_t* barrier) {
  barrier->countdown = countdown_value;
}

static inline void barrier_wait(barrier_t* barrier) {
  arch_spin_lock(&barrier->lock);
  barrier->countdown--;
  arch_spin_unlock(&barrier->lock);
  while (1) {
    arch_spin_lock(&barrier->lock);
    if (!barrier->countdown) {
      return;
    } else {
      arch_spin_unlock(&barrier->lock);
      for (int i = 0; i < 1000; i++) {
        asm volatile("nop");
      }
    }
  }
}

#endif  //__UTIL_H
