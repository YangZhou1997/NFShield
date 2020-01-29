#ifndef __DLEFT_HASH_H__
#define __DLEFT_HASH_H__

#ifdef	__cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "fnv64.h"
#include "common.h" 

#define BUCKET_SIZE 32
// the number of bucket_entry in each bucket_list.
#define BUCKET_MASK ((CAST_PTR(uint64_t, 1)<<BUCKET_SIZE) - 1)  // (1<<BZUCKET_SIZE) - 1
// used for operating the occupy_set
#define DLEFT 2
// the number of mapped bucket_list per key.

#ifdef VALUE_TYPE
    #if VALUE_TYPE == U32
        typedef uint32_t value_t;
    #elif VALUE_TYPE == U16
        typedef uint16_t value_t;
    #elif VALUE_TYPE == TUPLE
        typedef five_tuple_t value_t;
    #elif VALUE_TYPE == BOOL
        typedef bool value_t;
    #endif
#else
        typedef uint32_t value_t;
#endif


typedef struct
{
	five_tuple_t key;
	value_t value;
}bucket_entry_t;

typedef struct
{
	bucket_entry_t entry[BUCKET_SIZE];
}bucket_list_t;

typedef struct
{
    char table_name[MAX_TABLE_NAME_LEN];
    uint64_t block_ptr;
	
    // the number of bucket_list; should be less than 2^24, otherwise need to change aging table allocation
    uint32_t table_size;
	uint32_t *occupy_set; // length: table_size
	bucket_list_t *dleft; // length: table_size
}dleft_meta_t;

int dleft_init(char *table_name, uint32_t entry_num, dleft_meta_t *ht_ptr)
{   
    void * block_ptr;
    uint64_t whole_size, table_size; // qvq, whole_size cannot be uint32_t, since large memory will overflow it.

    table_size = entry_num / BUCKET_SIZE + 1;
    whole_size = table_size * sizeof(uint32_t) + table_size * sizeof(bucket_list_t);

#ifdef CAVIUM_PLATFORM
    cvmx_dprintf("value_t size: %luB\n", sizeof(value_t));
    cvmx_dprintf("memory size: %lfMB\n", whole_size * 1.0 / 1024 / 1024);
#else
    printf("value_t size: %luB\n", sizeof(value_t));
    printf("memory size: %lfMB\n", whole_size * 1.0 / 1024 / 1024);
#endif

    block_ptr = zmalloc(whole_size, ALIGN, table_name);
    if(block_ptr == NULL)
        return -1;
    
    strcpy(ht_ptr->table_name, table_name);
    ht_ptr->block_ptr = CAST_PTR(uint64_t, block_ptr);
    ht_ptr->table_size = table_size;
    ht_ptr->occupy_set = CAST_PTR(uint32_t *, block_ptr);
    ht_ptr->dleft = CAST_PTR(bucket_list_t *, CAST_PTR(char *, block_ptr) + table_size * sizeof(uint32_t));

    return 1;
}

int dleft_destroy(dleft_meta_t *ht_ptr)
{
#ifdef CAVIUM_PLATFORM
    zfree(ht_ptr->table_name);
    return 1;
#else
    zfree(CAST_PTR(void *, ht_ptr->block_ptr));
    return 1;
#endif
}

// On success, return the pointer to the value, otherwise return NULL;
value_t * dleft_lookup(dleft_meta_t *ht_ptr, five_tuple_t key)
{
    uint32_t update_hash_value[DLEFT];
    uint32_t bit_set, j;
    uint32_t idx;
    uint32_t *occupy_set = ht_ptr->occupy_set;
    bucket_list_t * dleft = ht_ptr->dleft;

    for(j = 0; j < DLEFT; j++)
    {
		key.proto |= ((uint8_t)j << 4);
        update_hash_value[j] = (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
		key.proto &= 0xF;

        // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1, &update_hash_value[j]);
        update_hash_value[j] %= ht_ptr->table_size;
        // printf("%x %x %hu %hu %hu\n", key.srcip, key.dstip, key.srcport, key.dstport, key.proto);        
        // printf("%u\n", update_hash_value[j]);

        bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);

        idx = 0;
        while(bit_set)
        {
            if(bit_set & 1)
            {
                if(IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key))
                {
					// printf("dleft_lookup\n");
                    // we find the key in the hash table; 
					// dleft[update_hash_value[j]].entry[idx].value = value;
                    return &(dleft[update_hash_value[j]].entry[idx].value);
                }
            }
            bit_set >>=1;
            idx++;
        }
    }

	return NULL;
}


// On success, return 1; otherwise, return -1;
int dleft_delete(dleft_meta_t *ht_ptr, five_tuple_t key)
{
    uint32_t update_hash_value[DLEFT];
    uint32_t bit_set;
    uint32_t idx, j;
    uint32_t *occupy_set = ht_ptr->occupy_set;
    bucket_list_t * dleft = ht_ptr->dleft;

    for(j = 0; j < DLEFT; j++)
    {
		key.proto |= ((uint8_t)j << 4);
        update_hash_value[j] = (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
		key.proto &= 0xF;

        // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1, &update_hash_value[j]);
        
        update_hash_value[j] %= ht_ptr->table_size;

        bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);

        idx = 0;
        while(bit_set)
        {
            if(bit_set & 1)
            {
                if(IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key))
                {
                    occupy_set[update_hash_value[j]] &= (~(1 << idx));
                    return 1;
                }
            }
            bit_set >>=1;
            idx++;
        }
    }
    return -1;
}

// return 1: find key;
// 0: do not find key but can insert;
// -1: do not find key and cannot insert (hashmap is full);
int dleft_update(dleft_meta_t *ht_ptr, five_tuple_t key, value_t value)
{
    uint32_t update_hash_value[DLEFT];
    uint32_t bit_set, j;
    uint32_t load, idx;
    uint32_t least_load, least_load_idx;
    uint32_t bitset_array[DLEFT];
    uint32_t *occupy_set = ht_ptr->occupy_set;
    bucket_list_t * dleft = ht_ptr->dleft;

    least_load_idx = 0;
    least_load = BUCKET_SIZE + 1;

    for(j = 0; j < DLEFT; j++)
    {
		key.proto |= ((uint8_t)j << 4);
        update_hash_value[j] = (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
		key.proto &= 0xF;

        // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1, &update_hash_value[j]);
        update_hash_value[j] %= ht_ptr->table_size;
        // printf("%x %x %hu %hu %hu\n", key.srcip, key.dstip, key.srcport, key.dstport, key.proto);
        // printf("%u\n", update_hash_value[j]);

        bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);
        bitset_array[j] = bit_set;

        load = 0;
        idx = 0;
        while(bit_set)
        {
            if(bit_set & 1)
            {
                if(IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key))
                {
                    // we find the key in the hash table; 
					dleft[update_hash_value[j]].entry[idx].value = value;
                    return 1;
                }
            }
            load += (bit_set & 1);
            bit_set >>=1;
            idx++;
        }
        if(least_load > load)
        {
            least_load = load;
            least_load_idx = j;
        }
    }

    // printf("%x %x %hu %hu %hu\n", key.srcip, key.dstip, key.srcport, key.dstport, key.proto);
	// printf("0 %u\n", update_hash_value[least_load_idx]);

    // we do not find the key;
    // the DLEFT bucket lists are not full! 
    if(least_load < BUCKET_SIZE)
    {   
        bit_set = bitset_array[least_load_idx];
        idx = 0;
    
        while(idx < BUCKET_SIZE)
        {
            if(!(bit_set & 1))
            {
                occupy_set[update_hash_value[least_load_idx]] |= (1 << idx);
				dleft[update_hash_value[least_load_idx]].entry[idx].key = key;
    			dleft[update_hash_value[least_load_idx]].entry[idx].value = value;
                return 0;
            }
            idx ++;
            bit_set >>= 1;
        }
    }
    else
    // the DLEFT bucket lists are full! 
    {
        return -1;
    }
    return -2;
}


// dleft_add_value() can only be called when value_t is uint32_t;
#ifdef VALUE_TYPE
#if VALUE_TYPE == U32

// return 1: find key;
// 0: do not find key, but insert key with default value of delta;
// -1: do not find key and cannot insert (hashmap is full);
int dleft_add_value(dleft_meta_t *ht_ptr, five_tuple_t key, uint32_t delta)
{
    uint32_t update_hash_value[DLEFT];
    uint32_t bit_set, j;
    uint32_t load, idx;
    uint32_t least_load, least_load_idx;
    uint32_t bitset_array[DLEFT];
    uint32_t *occupy_set = ht_ptr->occupy_set;
    bucket_list_t * dleft = ht_ptr->dleft;

    least_load_idx = 0;
    least_load = BUCKET_SIZE + 1;

    for(j = 0; j < DLEFT; j++)
    {
		key.proto |= ((uint8_t)j << 4);
        update_hash_value[j] = (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
		key.proto &= 0xF;

        // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1, &update_hash_value[j]);
        update_hash_value[j] %= ht_ptr->table_size;

        bit_set = occupy_set[update_hash_value[j]] & (BUCKET_MASK);
        bitset_array[j] = bit_set;

        load = 0;
        idx = 0;
        while(bit_set)
        {
            if(bit_set & 1)
            {
                if(IS_EQUAL(dleft[update_hash_value[j]].entry[idx].key, key))
                {
                    // we find the key in the hash table; 
					dleft[update_hash_value[j]].entry[idx].value += delta;
                    return 1;
                }
            }
            load += (bit_set & 1);
            bit_set >>=1;
            idx++;
        }
        if(least_load > load)
        {
            least_load = load;
            least_load_idx = j;
        }
    }

    // we do not find the key;
    // the DLEFT bucket lists are not full! 
    if(least_load < BUCKET_SIZE)
    {   
        bit_set = bitset_array[least_load_idx];
        idx = 0;
    
        while(idx < BUCKET_SIZE)
        {
            if(!(bit_set & 1))
            {
                occupy_set[update_hash_value[least_load_idx]] |= (1 << idx);
				dleft[update_hash_value[least_load_idx]].entry[idx].key = key;
    			dleft[update_hash_value[least_load_idx]].entry[idx].value = delta;
                return 0;
            }
            idx ++;
            bit_set >>= 1;
        }
    }
    else
    // the DLEFT bucket lists are full! 
    {
        return -1;
    }
    return -2;
}
#endif
#endif

#ifdef	__cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif /* __DLEFT_HASH_H__ */
