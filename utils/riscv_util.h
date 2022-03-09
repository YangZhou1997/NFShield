// See LICENSE for license details.

// this file is mostly copied from
// https://github.com/firesim/network-benchmarks/blob/c4945a77bff8af81d4b9af0daefec4ac0357dd51/common/util.h
// except for the barrier_t and related function implementation.

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

static void printArray(const char name[], int n, const int arr[]) {
#if HOST_DEBUG
  int i;
  printf(" %10s :", name);
  for (i = 0; i < n; i++) printf(" %3d ", arr[i]);
  printf("\n");
#endif
}

static void printDoubleArray(const char name[], int n, const double arr[]) {
#if HOST_DEBUG
  int i;
  printf(" %10s :", name);
  for (i = 0; i < n; i++) printf(" %g ", arr[i]);
  printf("\n");
#endif
}

static int verify(int n, const volatile int* test, const int* verify) {
  int i;
  // Unrolled for faster verification
  for (i = 0; i < n / 2 * 2; i += 2) {
    int t0 = test[i], t1 = test[i + 1];
    int v0 = verify[i], v1 = verify[i + 1];
    if (t0 != v0) return i + 1;
    if (t1 != v1) return i + 2;
  }
  if (n % 2 != 0 && test[n - 1] != verify[n - 1]) return n;
  return 0;
}

static int verifyDouble(int n, const volatile double* test,
                        const double* verify) {
  int i;
  // Unrolled for faster verification
  for (i = 0; i < n / 2 * 2; i += 2) {
    double t0 = test[i], t1 = test[i + 1];
    double v0 = verify[i], v1 = verify[i + 1];
    int eq1 = t0 == v0, eq2 = t1 == v1;
    if (!(eq1 & eq2)) return i + 1 + eq1;
  }
  if (n % 2 != 0 && test[n - 1] != verify[n - 1]) return n;
  return 0;
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

static uint64_t lfsr(uint64_t x) {
  uint64_t bit = (x ^ (x >> 1)) & 1;
  return (x >> 1) | (bit << 62);
}

static uint16_t compute_checksum(uint16_t* data, int n, uint16_t init) {
  uint32_t sum = init;

  for (int i = 0; i < n; i++) sum += data[i];

  while ((sum >> 16) != 0) sum = (sum & 0xffff) + (sum >> 16);

  return ~sum & 0xffff;
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
      arch_spin_unlock(&barrier->lock);
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
