// See LICENSE for license details.

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>
#include <time.h>

#include "riscv_util.h"

#define SYS_write 64

#undef strcmp

extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

#define NCORES 4

arch_spinlock_t exit_lock, print_lock, alloc_lock;
int num_exited = 0;

void co_exit(int err) {
  arch_spin_lock(&exit_lock);
  num_exited++;
  arch_spin_unlock(&exit_lock);
  while (1) {
    arch_spin_lock(&exit_lock);
    if (num_exited == NCORES) {
      // directly die, no need to release
      exit(err);
    } else {
      arch_spin_unlock(&exit_lock);
      for (int i = 0; i < 1000; i++) {
        asm volatile("nop");
      }
    }
  }
}

static uintptr_t syscall(uintptr_t which, uint64_t arg0, uint64_t arg1,
                         uint64_t arg2) {
  volatile uint64_t magic_mem[8] __attribute__((aligned(64)));
  magic_mem[0] = which;
  magic_mem[1] = arg0;
  magic_mem[2] = arg1;
  magic_mem[3] = arg2;
  __sync_synchronize();

  tohost = (uintptr_t)magic_mem;
  while (fromhost == 0)
    ;
  fromhost = 0;

  __sync_synchronize();
  return magic_mem[0];
}

#define NUM_COUNTERS 2
static uintptr_t counters[NUM_COUNTERS];
static char* counter_names[NUM_COUNTERS];

void setStats(int enable) {
  int i = 0;
#define READ_CTR(name)              \
  do {                              \
    while (i >= NUM_COUNTERS)       \
      ;                             \
    uintptr_t csr = read_csr(name); \
    if (!enable) {                  \
      csr -= counters[i];           \
      counter_names[i] = #name;     \
    }                               \
    counters[i++] = csr;            \
  } while (0)

  READ_CTR(mcycle);
  READ_CTR(minstret);

#undef READ_CTR
}

void __attribute__((noreturn)) tohost_exit(uintptr_t code) {
  tohost = (code << 1) | 1;
  while (1)
    ;
}

uintptr_t __attribute__((weak))
handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32]) {
  tohost_exit(1337);
}

void exit(int code) { tohost_exit(code); }

void abort() { exit(128 + SIGABRT); }

void printstr(const char* s) { syscall(SYS_write, 1, (uintptr_t)s, strlen(s)); }

// only allow multi-threaded program
void __attribute__((weak)) thread_entry(int cid, int nc) {
  // multi-threaded programs override this function.
  // for the case of single-threaded programs, only let core 0 proceed.
  //   while (cid != 0);
  printstr("Implement thread_entry(), foo!\n");
  return;
}

// int __attribute__((weak)) main(int argc, char** argv)
// {
//   // single-threaded programs override this function.
//   printstr("Implement main(), foo!\n");
//   return -1;
// }

static void init_tls() {
  register void* thread_pointer asm("tp");
  extern char _tls_data;
  extern __thread char _tdata_begin, _tdata_end, _tbss_end;
  size_t tdata_size = &_tdata_end - &_tdata_begin;
  memcpy(thread_pointer, &_tls_data, tdata_size);
  size_t tbss_size = &_tbss_end - &_tdata_end;
  memset(thread_pointer + tdata_size, 0, tbss_size);
}

void _init(int cid, int nc) {
  init_tls();
  thread_entry(cid, nc);
  co_exit(0);

  //   // only single-threaded programs should ever get here.
  //   int ret = main(0, 0);

  //   char buf[NUM_COUNTERS * 32] __attribute__((aligned(64)));
  //   char* pbuf = buf;
  //   for (int i = 0; i < NUM_COUNTERS; i++)
  //     if (counters[i])
  //       pbuf += sprintf(pbuf, "%s = %d\n", counter_names[i], counters[i]);
  //   if (pbuf != buf)
  //     printstr(buf);

  //   exit(ret);
}

#undef putchar
int putchar(int ch) {
  static __thread char buf[64] __attribute__((aligned(64)));
  static __thread int buflen = 0;

  buf[buflen++] = ch;

  if (ch == '\n' || buflen == sizeof(buf)) {
    syscall(SYS_write, 1, (uintptr_t)buf, buflen);
    buflen = 0;
  }

  return 0;
}

void printhex(uint64_t x) {
  char str[17];
  int i;
  for (i = 0; i < 16; i++) {
    str[15 - i] = (x & 0xF) + ((x & 0xF) < 10 ? '0' : 'a' - 10);
    x >>= 4;
  }
  str[16] = 0;

  printstr(str);
}

static inline void printnum(void (*putch)(int, void**), void** putdat,
                            unsigned long long num, unsigned base, int width,
                            int padc) {
  unsigned digs[sizeof(num) * CHAR_BIT];
  int pos = 0;

  while (1) {
    digs[pos++] = num % base;
    if (num < base) break;
    num /= base;
  }

  while (width-- > pos) putch(padc, putdat);

  while (pos-- > 0)
    putch(digs[pos] + (digs[pos] >= 10 ? 'a' - 10 : '0'), putdat);
}

static unsigned long long getuint(va_list* ap, int lflag) {
  if (lflag >= 2)
    return va_arg(*ap, unsigned long long);
  else if (lflag)
    return va_arg(*ap, unsigned long);
  else
    return va_arg(*ap, unsigned int);
}

static long long getint(va_list* ap, int lflag) {
  if (lflag >= 2)
    return va_arg(*ap, long long);
  else if (lflag)
    return va_arg(*ap, long);
  else
    return va_arg(*ap, int);
}

static void vprintfmt(void (*putch)(int, void**), void** putdat,
                      const char* fmt, va_list ap) {
  register const char* p;
  const char* last_fmt;
  register int ch, err;
  unsigned long long num;
  int base, lflag, width, precision, altflag;
  char padc;

  while (1) {
    while ((ch = *(unsigned char*)fmt) != '%') {
      if (ch == '\0') return;
      fmt++;
      putch(ch, putdat);
    }
    fmt++;

    // Process a %-escape sequence
    last_fmt = fmt;
    padc = ' ';
    width = -1;
    precision = -1;
    lflag = 0;
    altflag = 0;
  reswitch:
    switch (ch = *(unsigned char*)fmt++) {
      // flag to pad on the right
      case '-':
        padc = '-';
        goto reswitch;

      // flag to pad with 0's instead of spaces
      case '0':
        padc = '0';
        goto reswitch;

      // width field
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        for (precision = 0;; ++fmt) {
          precision = precision * 10 + ch - '0';
          ch = *fmt;
          if (ch < '0' || ch > '9') break;
        }
        goto process_precision;

      case '*':
        precision = va_arg(ap, int);
        goto process_precision;

      case '.':
        if (width < 0) width = 0;
        goto reswitch;

      case '#':
        altflag = 1;
        goto reswitch;

      process_precision:
        if (width < 0) width = precision, precision = -1;
        goto reswitch;

      // long flag (doubled for long long)
      case 'l':
        lflag++;
        goto reswitch;

      // character
      case 'c':
        putch(va_arg(ap, int), putdat);
        break;

      // string
      case 's':
        if ((p = va_arg(ap, char*)) == NULL) p = "(null)";
        if (width > 0 && padc != '-')
          for (width -= strnlen(p, precision); width > 0; width--)
            putch(padc, putdat);
        for (; (ch = *p) != '\0' && (precision < 0 || --precision >= 0);
             width--) {
          putch(ch, putdat);
          p++;
        }
        for (; width > 0; width--) putch(' ', putdat);
        break;

      // (signed) decimal
      case 'd':
        num = getint(&ap, lflag);
        if ((long long)num < 0) {
          putch('-', putdat);
          num = -(long long)num;
        }
        base = 10;
        goto signed_number;

      // unsigned decimal
      case 'u':
        base = 10;
        goto unsigned_number;

      // (unsigned) octal
      case 'o':
        // should do something with padding so it's always 3 octits
        base = 8;
        goto unsigned_number;

      // pointer
      case 'p':
        static_assert(sizeof(long) == sizeof(void*));
        lflag = 1;
        putch('0', putdat);
        putch('x', putdat);
        /* fall through to 'x' */

      // (unsigned) hexadecimal
      case 'x':
        base = 16;
      unsigned_number:
        num = getuint(&ap, lflag);
      signed_number:
        printnum(putch, putdat, num, base, width, padc);
        break;

      // escaped '%' character
      case '%':
        putch(ch, putdat);
        break;

      // unrecognized escape sequence - just print it literally
      default:
        putch('%', putdat);
        fmt = last_fmt;
        break;
    }
  }
}

int printf(const char* fmt, ...) {
  arch_spin_lock(&print_lock);
  va_list ap;
  va_start(ap, fmt);

  vprintfmt((void*)putchar, 0, fmt, ap);

  va_end(ap);
  arch_spin_unlock(&print_lock);
  return 0;  // incorrect return value, but who cares, anyway?
}

int sprintf(char* str, const char* fmt, ...) {
  va_list ap;
  char* str0 = str;
  va_start(ap, fmt);

  void sprintf_putch(int ch, void** data) {
    char** pstr = (char**)data;
    **pstr = ch;
    (*pstr)++;
  }

  vprintfmt(sprintf_putch, (void**)&str, fmt, ap);
  *str = 0;

  va_end(ap);
  return str - str0;
}

void* memcpy(void* dest, const void* src, size_t len) {
  if ((((uintptr_t)dest | (uintptr_t)src | len) & (sizeof(uintptr_t) - 1)) ==
      0) {
    const uintptr_t* s = src;
    uintptr_t* d = dest;
    while (d < (uintptr_t*)(dest + len)) *d++ = *s++;
  } else {
    const char* s = src;
    char* d = dest;
    while (d < (char*)(dest + len)) *d++ = *s++;
  }
  return dest;
}

void* memset(void* dest, int byte, size_t len) {
  if ((((uintptr_t)dest | len) & (sizeof(uintptr_t) - 1)) == 0) {
    uintptr_t word = byte & 0xFF;
    word |= word << 8;
    word |= word << 16;
    word |= word << 16 << 16;

    uintptr_t* d = dest;
    while (d < (uintptr_t*)(dest + len)) *d++ = word;
  } else {
    char* d = dest;
    while (d < (char*)(dest + len)) *d++ = byte;
  }
  return dest;
}

size_t strlen(const char* s) {
  const char* p = s;
  while (*p) p++;
  return p - s;
}

size_t strnlen(const char* s, size_t n) {
  const char* p = s;
  while (n-- && *p) p++;
  return p - s;
}

int strcmp(const char* s1, const char* s2) {
  unsigned char c1, c2;

  do {
    c1 = *s1++;
    c2 = *s2++;
  } while (c1 != 0 && c1 == c2);

  return c1 - c2;
}

char* strcpy(char* dest, const char* src) {
  char* d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

long atol(const char* str) {
  long res = 0;
  int sign = 0;

  while (*str == ' ') str++;

  if (*str == '-' || *str == '+') {
    sign = *str == '-';
    str++;
  }

  while (*str) {
    res *= 10;
    res += *str++ - '0';
  }

  return sign ? -res : res;
}

static __thread uint64_t random_state;

void srandom(unsigned int seed) {
  random_state = seed;
  random_state |= (random_state << 32);
}

uint64_t random() {
  uint64_t x = random_state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x << 27;
  random_state = x;
  return x;
}

time_t time(time_t* tloc) {
  uint64_t usecs, secs;

  asm volatile("rdtime %[usecs]" : [usecs] "=r"(usecs));

  secs = usecs / 1000000;

  if (tloc != NULL) *tloc = secs;
  return secs;
}

// the following malloc is borrowed from
// https://github.com/embeddedartistry/libmemory
/*
 * Copyright © 2017 Embedded Artistry LLC.
 * License: MIT. See LICENSE file for details.
 */

#include <memory.h>
#include <stdint.h>

#include "ll.h"

/**
 * Simple macro for making sure memory addresses are aligned
 * to the nearest power of two
 */
#ifndef align_up
#define align_up(num, align) (((num) + ((align)-1)) & ~((align)-1))
#endif

/*
 * This is the container for our free-list.
 * Node the usage of the linked list here: the library uses offsetof
 * and container_of to manage the list and get back to the parent struct.
 */
typedef struct {
  ll_t node;
  size_t size;
  char* block;
} alloc_node_t;

/**
 * We vend a memory address to the user.  This lets us translate back and forth
 * between the vended pointer and the container we use for managing the data.
 */
#define ALLOC_HEADER_SZ offsetof(alloc_node_t, block)

// We are enforcing a minimum allocation size of 32B.
#define MIN_ALLOC_SZ ALLOC_HEADER_SZ + 32

static void defrag_free_list(void);

static inline void malloc_lock() { arch_spin_lock(&alloc_lock); }

static inline void malloc_unlock() { arch_spin_unlock(&alloc_lock); }

// This macro simply declares and initializes our linked list
static LIST_INIT(free_list);

/**
 * When we free, we can take our node and check to see if any memory blocks
 * can be combined into larger blocks.  This will help us fight against
 * memory fragmentation in a simple way.
 */
void defrag_free_list(void) {
  alloc_node_t* b = NULL;
  alloc_node_t* lb = NULL;
  alloc_node_t* t = NULL;

  list_for_each_entry_safe(b, t, &free_list, node) {
    if (lb) {
      if ((((uintptr_t)&lb->block) + lb->size) == (uintptr_t)b) {
        lb->size += sizeof(*b) + b->size;
        list_del(&b->node);
        continue;
      }
    }
    lb = b;
  }
}

void* malloc(size_t size) {
  void* ptr = NULL;
  alloc_node_t* blk = NULL;

  if (size > 0) {
    // Align the pointer
    size = align_up(size, sizeof(void*));

    malloc_lock();

    // try to find a big enough block to alloc
    list_for_each_entry(blk, &free_list, node) {
      if (blk->size >= size) {
        ptr = &blk->block;
        break;
      }
    }

    // we found something
    if (ptr) {
      // Can we split the block?
      if ((blk->size - size) >= MIN_ALLOC_SZ) {
        alloc_node_t* new_blk =
            (alloc_node_t*)((uintptr_t)(&blk->block) + size);
        new_blk->size = blk->size - size - ALLOC_HEADER_SZ;
        blk->size = size;
        list_insert(&new_blk->node, &blk->node, blk->node.next);
      }

      list_del(&blk->node);
    }

    malloc_unlock();

  }  // else NULL

  return ptr;
}

void* calloc(size_t n, size_t size) {
  printf("calloc a\n");
  void* p = malloc(n * size);
  printf("calloc b\n");
  memset(p, 0, n * size);
  printf("calloc c\n");
  return p;
}

void free(void* ptr) {
  // Don't free a NULL pointer..
  if (ptr) {
    // we take the pointer and use container_of to get the corresponding alloc
    // block
    alloc_node_t* blk = container_of(ptr, alloc_node_t, block);
    alloc_node_t* free_blk = NULL;

    malloc_lock();

    // Let's put it back in the proper spot
    list_for_each_entry(free_blk, &free_list, node) {
      if (free_blk > blk) {
        list_insert(&blk->node, free_blk->node.prev, &free_blk->node);
        goto blockadded;
      }
    }
    list_add_tail(&blk->node, &free_list);

  blockadded:
    // Let's see if we can combine any memory
    defrag_free_list();

    malloc_unlock();
  }
}

/**
 * @brief Assign blocks of memory for use by malloc().
 *
 * Initializes the malloc() backend with a memory address and memory pool size.
 *	This memory is assumed to be owned by malloc() and is vended out when
 *memory is requested. Multiple blocks can be added.
 *
 * NOTE: This API must be called before malloc() can be used. If you call
 *malloc() before allocating memory, malloc() will return NULL because there is
 *no available memory to provide to the user.
 *
 * @param addr Pointer to the memory block address that you are providing to
 *malloc()
 * @param size Size of the memory block that you are providing to malloc()
 */

void malloc_addblock(void* addr, size_t size) {
  // let's align the start address of our block to the next pointer aligned
  // number
  alloc_node_t* blk = (void*)align_up((uintptr_t)addr, sizeof(void*));

  // calculate actual size - remove our alignment and our header space from the
  // availability
  blk->size = (uintptr_t)addr + size - (uintptr_t)blk - ALLOC_HEADER_SZ;

  // and now our giant block of memory is added to the list!
  malloc_lock();
  list_add(&blk->node, &free_list);
  malloc_unlock();
}

#include <stdio.h>
FILE* fopen(const char* filename, const char* mode) {
  printf("fopen not implemented");
  exit(0);
}
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
  printf("fwrite not implemented");
  exit(0);
}
int fclose(FILE* stream) {
  printf("fclose not implemented");
  exit(0);
}

// the following is borrowed from https://github.com/aethaniel/minilib-c

int isdigit(int c) { return (unsigned)c - '0' < 10; }
int isprint(int c) { return (c >= 0x20 && c <= 0x7E); }
int isascii(int c) { return (c >= 0 && c < 128); }
int islower(int c) { return ((c >= 'a') && (c <= 'z')); }
int toupper(int c) { return islower(c) ? c - 'a' + 'A' : c; }
int isupper(int c) { return ((c >= 'A') && (c <= 'Z')); }
int isalpha(int c) {
  return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}
int isxdigit(int c) {
  return (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'F')) ||
          ((c >= 'a') && (c <= 'f')));
}
int isspace(int c) { return ((c >= 0x09 && c <= 0x0D) || (c == 0x20)); }

long strtol(const char* nptr, char** ptr, int base) {
  register const char* s = nptr;
  register unsigned long acc;
  register int c;
  register unsigned long cutoff;
  register int neg = 0, any, cutlim;

  /*
   * Skip white space and pick up leading +/- sign if any.
   * If base is 0, allow 0x for hex and 0 for octal, else
   * assume decimal; if base is already 16, allow 0x.
   */
  do {
    c = *s++;
  } while (isspace(c));

  if (c == '-') {
    neg = 1;
    c = *s++;
  } else if (c == '+')
    c = *s++;

  if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
    c = s[1];
    s += 2;
    base = 16;
  }

  if (base == 0) base = c == '0' ? 8 : 10;

  /*
   * Compute the cutoff value between legal numbers and illegal
   * numbers.  That is the largest legal value, divided by the
   * base.  An input number that is greater than this value, if
   * followed by a legal input character, is too big.  One that
   * is equal to this value may be valid or not; the limit
   * between valid and invalid numbers is then based on the last
   * digit.  For instance, if the range for longs is
   * [-2147483648..2147483647] and the input base is 10,
   * cutoff will be set to 214748364 and cutlim to either
   * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
   * a value > 214748364, or equal but the next digit is > 7 (or 8),
   * the number is too big, and we will return a range error.
   *
   * Set any if any `digits' consumed; make it negative to indicate
   * overflow.
   */

  cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
  cutlim = cutoff % (unsigned long)base;
  cutoff /= (unsigned long)base;

  for (acc = 0, any = 0;; c = *s++) {
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base) break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0) {
    acc = neg ? LONG_MIN : LONG_MAX;
  } else if (neg)
    acc = -acc;
  if (ptr != 0) *ptr = (char*)(any ? s - 1 : nptr);
  return (acc);
}

char* scan_string(const char* str, int base) {
  unsigned char* str_ptr = (unsigned char*)str;

  switch (base) {
    case 10:
      while (!(isdigit(*str_ptr) || *str_ptr == '-' || *str_ptr == 0x0)) {
        str_ptr++;
      }
      break;
    case 16:
      while (!(isxdigit(*str_ptr) || *str_ptr == 0x0)) {
        str_ptr++;
      }
      break;
  }

  return (char*)str_ptr;
}

int sscanf(const char* str, const char* fmt, ...) {
  int ret;
  va_list ap;
  char* format_ptr = (char*)fmt;
  char* str_ptr = (char*)str;

  int* p_int;
  long* p_long;

  ret = 0;

  va_start(ap, fmt);

  while ((*format_ptr != 0x0) && (*str_ptr != 0x0)) {
    if (*format_ptr == '%') {
      format_ptr++;

      if (*format_ptr != 0x0) {
        switch (*format_ptr) {
          case 'd':  // expect an int
          case 'i':
            p_int = va_arg(ap, int*);
            str_ptr = scan_string(str_ptr, 10);
            if (*str_ptr == 0x0) goto end_parse;
            *p_int = (int)strtol(str_ptr, &str_ptr, 10);
            ret++;
            break;
          case 'D':
          case 'I':  // expect a long
            p_long = va_arg(ap, long*);
            str_ptr = scan_string(str_ptr, 10);
            if (*str_ptr == 0x0) goto end_parse;
            *p_long = strtol(str_ptr, &str_ptr, 10);
            ret++;
            break;
          case 'x':  // expect an int in hexadecimal format
            p_int = va_arg(ap, int*);
            str_ptr = scan_string(str_ptr, 16);
            if (*str_ptr == 0x0) goto end_parse;
            *p_int = (int)strtol(str_ptr, &str_ptr, 16);
            ret++;
            break;
          case 'X':  // expect a long in hexadecimal format
            p_long = va_arg(ap, long*);
            str_ptr = scan_string(str_ptr, 16);
            if (*str_ptr == 0x0) goto end_parse;
            *p_long = strtol(str_ptr, &str_ptr, 16);
            ret++;
            break;
        }
      }
    }

    format_ptr++;
  }

end_parse:
  va_end(ap);

  if (*str_ptr == 0x0) ret = EOF;
  return ret;
}

// borrowed from
// https://github.com/embeddedartistry/libc/blob/master/src/string/

char* __strtok_r(char* /*s*/ /*s*/, const char* /*delim*/ /*delim*/,
                 char** /*last*/ /*last*/);

char* __strtok_r(char* s, const char* delim, char** last) {
  char *spanp, *tok;
  int c, sc;

  if (s == NULL && (s = *last) == NULL) {
    { return (NULL); }
  }

/*
 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
 */
cont:
  c = *s++;
  for (spanp = (char*)(uintptr_t)delim; (sc = *spanp++) != 0;) {
    if (c == sc) {
      { goto cont; }
    }
  }

  if (c == 0) { /* no non-delimiter characters */
    *last = NULL;
    return (NULL);
  }
  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;) {
    c = *s++;
    spanp = (char*)(uintptr_t)delim;
    do {
      if ((sc = *spanp++) == c) {
        if (c == 0) {
          { s = NULL; }
        } else {
          { s[-1] = '\0'; }
        }
        *last = s;
        return (tok);
      }
    } while (sc != 0);
  }
  /* NOTREACHED */
}

char* strtok(char* s, const char* delim) {
  static char* last;

  return (__strtok_r(s, delim, &last));
}