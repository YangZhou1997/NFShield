#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fnv64.h"

#ifndef TYPE
#define TYPE uint32_t
#define TYPE_STR U32
#define TYPED_NAME(x) u32_##x
#endif

#ifndef BUCKE_SIZE
// the number of bucket_entry in each bucket_list.
#define BUCKET_SIZE 32
// used for operating the occupy_set
#define BUCKET_MASK \
  ((CAST_PTR(uint64_t, 1) << BUCKET_SIZE) - 1)  // (1<<BZUCKET_SIZE) - 1
// the number of mapped bucket_list per key.
#define DLEFT 2
#endif

typedef struct {
  five_tuple_t key;
  TYPE value;
} TYPED_NAME(bucket_entry_t);

typedef struct {
  TYPED_NAME(bucket_entry_t) entry[BUCKET_SIZE];
} TYPED_NAME(bucket_list_t);

typedef struct {
  char table_name[MAX_TABLE_NAME_LEN];
  uint64_t block_ptr;

  // the number of bucket_list; should be less than 2^24, otherwise need to
  // change aging table allocation
  uint32_t table_size;
  uint32_t *occupy_set;               // length: table_size
  TYPED_NAME(bucket_list_t) * dleft;  // length: table_size
} TYPED_NAME(dleft_meta_t);

int TYPED_NAME(dleft_init)(char *table_name, uint32_t entry_num,
                           TYPED_NAME(dleft_meta_t) * ht_ptr) {
  void *block_ptr;
  uint64_t whole_size, table_size;  // qvq, whole_size cannot be uint32_t, since
                                    // large memory will overflow it.

  table_size = entry_num / BUCKET_SIZE + 1;
  whole_size = table_size * sizeof(uint32_t) +
               table_size * sizeof(TYPED_NAME(bucket_list_t));

#ifdef CAVIUM_PLATFORM
  cvmx_dprintf("TYPE size: %luB\n", sizeof(TYPE));
  cvmx_dprintf("memory size: %lfMB\n", whole_size * 1.0 / 1024 / 1024);
#else
  printf("TYPE size: %luB\n", sizeof(TYPE));
  printf("memory size: %uMB\n", (uint32_t)(whole_size * 1.0 / 1024 / 1024));
#endif

  block_ptr = malloc(whole_size);
  if (block_ptr == NULL) return -1;

  strcpy(ht_ptr->table_name, table_name);
  ht_ptr->block_ptr = CAST_PTR(uint64_t, block_ptr);
  ht_ptr->table_size = table_size;
  ht_ptr->occupy_set = CAST_PTR(uint32_t *, block_ptr);
  ht_ptr->dleft =
      CAST_PTR(TYPED_NAME(bucket_list_t) *,
               CAST_PTR(char *, block_ptr) + table_size * sizeof(uint32_t));

  return 1;
}

int TYPED_NAME(dleft_destroy)(TYPED_NAME(dleft_meta_t) * ht_ptr) {
#ifdef CAVIUM_PLATFORM
  zfree(ht_ptr->table_name);
  return 1;
#else
  zfree(CAST_PTR(void *, ht_ptr->block_ptr));
  return 1;
#endif
}

// On success, return the pointer to the value, otherwise return NULL;
TYPE *TYPED_NAME(dleft_lookup)(TYPED_NAME(dleft_meta_t) * ht_ptr,
                               five_tuple_t key) {
  uint32_t update_hash_value[DLEFT];
  uint32_t bit_set, j;
  uint32_t idx;
  uint32_t *occupy_set = ht_ptr->occupy_set;
  TYPED_NAME(bucket_list_t) *dleft = ht_ptr->dleft;

  for (j = 0; j < DLEFT; j++) {
    key.proto |= ((uint8_t)j << 4);
    update_hash_value[j] =
        (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
    key.proto &= 0xF;

    // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1,
    // &update_hash_value[j]);
    update_hash_value[j] %= ht_ptr->table_size;
    // printf("%x %x %hu %hu %hu\n", key.srcip, key.dstip, key.srcport,
    // key.dstport, key.proto); printf("%u\n", update_hash_value[j]);

    bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);

    idx = 0;
    while (bit_set) {
      if (bit_set & 1) {
        if (IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key)) {
          // printf("TYPED_NAME(dleft_lookup)\n");
          // we find the key in the hash table;
          // dleft[update_hash_value[j]].entry[idx].value = value;
          return &(dleft[update_hash_value[j]].entry[idx].value);
        }
      }
      bit_set >>= 1;
      idx++;
    }
  }

  return NULL;
}

#if TYPE_STR == BOOL
void TYPED_NAME(dleft_dump)(TYPED_NAME(dleft_meta_t) * ht_ptr, char *filename) {
  FILE *file = fopen(filename, "w");

  uint32_t *occupy_set = ht_ptr->occupy_set;
  TYPED_NAME(bucket_list_t) *dleft = ht_ptr->dleft;

  for (int i = 0; i < ht_ptr->table_size; i++) {
    uint32_t bit_set = occupy_set[i] & (BUCKET_MASK);

    uint32_t idx = 0;
    while (bit_set) {
      if (bit_set & 1) {
        five_tuple_t flow = dleft[i].entry[idx].key;
        uint8_t val = dleft[i].entry[idx].value ? 1 : 0;
        fwrite(&flow.srcip, sizeof(uint32_t), 1, file);
        fwrite(&flow.dstip, sizeof(uint32_t), 1, file);
        fwrite(&flow.srcport, sizeof(uint16_t), 1, file);
        fwrite(&flow.dstport, sizeof(uint16_t), 1, file);
        fwrite(&flow.proto, sizeof(uint8_t), 1, file);
        fwrite(&val, sizeof(uint8_t), 1, file);
      }
      bit_set >>= 1;
      idx++;
    }
  }
  fclose(file);
}
#endif

// On success, return 1; otherwise, return -1;
int TYPED_NAME(dleft_delete)(TYPED_NAME(dleft_meta_t) * ht_ptr,
                             five_tuple_t key) {
  uint32_t update_hash_value[DLEFT];
  uint32_t bit_set;
  uint32_t idx, j;
  uint32_t *occupy_set = ht_ptr->occupy_set;
  TYPED_NAME(bucket_list_t) *dleft = ht_ptr->dleft;

  for (j = 0; j < DLEFT; j++) {
    key.proto |= ((uint8_t)j << 4);
    update_hash_value[j] =
        (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
    key.proto &= 0xF;

    // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1,
    // &update_hash_value[j]);

    update_hash_value[j] %= ht_ptr->table_size;

    bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);

    idx = 0;
    while (bit_set) {
      if (bit_set & 1) {
        if (IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key)) {
          occupy_set[update_hash_value[j]] &= (~(1 << idx));
          return 1;
        }
      }
      bit_set >>= 1;
      idx++;
    }
  }
  return -1;
}

// return 1: find key;
// 0: do not find key but can insert;
// -1: do not find key and cannot insert (hashmap is full);
int TYPED_NAME(dleft_update)(TYPED_NAME(dleft_meta_t) * ht_ptr,
                             five_tuple_t key, TYPE value) {
  uint32_t update_hash_value[DLEFT];
  uint32_t bit_set, j;
  uint32_t load, idx;
  uint32_t least_load, least_load_idx;
  uint32_t bitset_array[DLEFT];
  uint32_t *occupy_set = ht_ptr->occupy_set;
  TYPED_NAME(bucket_list_t) *dleft = ht_ptr->dleft;

  least_load_idx = 0;
  least_load = BUCKET_SIZE + 1;

  for (j = 0; j < DLEFT; j++) {
    key.proto |= ((uint8_t)j << 4);
    update_hash_value[j] =
        (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
    key.proto &= 0xF;

    // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1,
    // &update_hash_value[j]);
    update_hash_value[j] %= ht_ptr->table_size;
    // printf("%x %x %hu %hu %hu\n", key.srcip, key.dstip, key.srcport,
    // key.dstport, key.proto); printf("%u\n", update_hash_value[j]);

    bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);
    bitset_array[j] = bit_set;

    load = 0;
    idx = 0;
    while (bit_set) {
      if (bit_set & 1) {
        if (IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key)) {
          // we find the key in the hash table;
          dleft[update_hash_value[j]].entry[idx].value = value;
          return 1;
        }
      }
      load += (bit_set & 1);
      bit_set >>= 1;
      idx++;
    }
    if (least_load > load) {
      least_load = load;
      least_load_idx = j;
    }
  }

  // printf("%x %x %hu %hu %hu\n", key.srcip, key.dstip, key.srcport,
  // key.dstport, key.proto); printf("0 %u\n",
  // update_hash_value[least_load_idx]);

  // we do not find the key;
  // the DLEFT bucket lists are not full!
  if (least_load < BUCKET_SIZE) {
    bit_set = bitset_array[least_load_idx];
    idx = 0;

    while (idx < BUCKET_SIZE) {
      if (!(bit_set & 1)) {
        occupy_set[update_hash_value[least_load_idx]] |= (1 << idx);
        dleft[update_hash_value[least_load_idx]].entry[idx].key = key;
        dleft[update_hash_value[least_load_idx]].entry[idx].value = value;
        return 0;
      }
      idx++;
      bit_set >>= 1;
    }
  } else
  // the DLEFT bucket lists are full!
  {
    return -1;
  }
  return -2;
}

// TYPED_NAME(dleft_add_value)() can only be called when TYPE is uint32_t;
#if TYPE_STR == U32

// return 1: find key;
// 0: do not find key, but insert key with default value of delta;
// -1: do not find key and cannot insert (hashmap is full);
int TYPED_NAME(dleft_add_value)(TYPED_NAME(dleft_meta_t) * ht_ptr,
                                five_tuple_t key, uint32_t delta) {
  uint32_t update_hash_value[DLEFT];
  uint32_t bit_set, j;
  uint32_t load, idx;
  uint32_t least_load, least_load_idx;
  uint32_t bitset_array[DLEFT];
  uint32_t *occupy_set = ht_ptr->occupy_set;
  TYPED_NAME(bucket_list_t) *dleft = ht_ptr->dleft;

  least_load_idx = 0;
  least_load = BUCKET_SIZE + 1;

  for (j = 0; j < DLEFT; j++) {
    key.proto |= ((uint8_t)j << 4);
    update_hash_value[j] =
        (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
    key.proto &= 0xF;

    // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1,
    // &update_hash_value[j]);
    update_hash_value[j] %= ht_ptr->table_size;

    bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);
    bitset_array[j] = bit_set;

    load = 0;
    idx = 0;
    while (bit_set) {
      if (bit_set & 1) {
        if (IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key)) {
          // we find the key in the hash table;
          dleft[update_hash_value[j]].entry[idx].value += delta;
          return 1;
        }
      }
      load += (bit_set & 1);
      bit_set >>= 1;
      idx++;
    }
    if (least_load > load) {
      least_load = load;
      least_load_idx = j;
    }
  }

  // we do not find the key;
  // the DLEFT bucket lists are not full!
  if (least_load < BUCKET_SIZE) {
    bit_set = bitset_array[least_load_idx];
    idx = 0;

    while (idx < BUCKET_SIZE) {
      if (!(bit_set & 1)) {
        occupy_set[update_hash_value[least_load_idx]] |= (1 << idx);
        dleft[update_hash_value[least_load_idx]].entry[idx].key = key;
        dleft[update_hash_value[least_load_idx]].entry[idx].value = delta;
        return 0;
      }
      idx++;
      bit_set >>= 1;
    }
  } else
  // the DLEFT bucket lists are full!
  {
    return -1;
  }
  return -2;
}
#endif
