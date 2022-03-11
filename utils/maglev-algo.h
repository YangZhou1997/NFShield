#ifndef __MAGLEV_HASH_H__
#define __MAGLEV_HASH_H__

#ifdef	__cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "fnv64.h"
#include "common.h"

#define HEALTHY 0x1
#define FAIL 0x0

typedef struct
{
    char table_name[MAX_TABLE_NAME_LEN];
    uint64_t block_ptr;

    uint32_t lookup_entry_num; // mush be prime number
    uint32_t backend_num;

    int *lookup_entry; // length: lookup_entry_num; record the index of backend in the backend_ip table; 
    uint32_t *backend_ip; // length: backend_num; record the ip of each backend server; 

    uint32_t *permutation; // length: backend_num * lookup_entry_num;
    uint8_t *backend_status; // length: backend_num;
    uint32_t *next; // length: backend_num
}maglev_meta_t;


void maglev_update_backend(maglev_meta_t *mag_ptr, uint32_t *backend_ip, uint8_t *backend_status);
void maglev_populate_lookup_entry(maglev_meta_t *mag_ptr);

int maglev_init_inner(char *table_name, uint32_t lookup_entry_num, uint32_t backend_num, uint32_t *backend_ip, uint8_t *backend_status, maglev_meta_t *mag_ptr)
{
    void *block_ptr;
    uint32_t total_size;

    total_size = lookup_entry_num * sizeof(int) + backend_num * (sizeof(uint32_t) + sizeof(uint32_t) * lookup_entry_num + sizeof(uint8_t) + sizeof(uint32_t));

    block_ptr = zmalloc(total_size, ALIGN, table_name);
    if(block_ptr == NULL)
        return -1;

    strcpy(mag_ptr->table_name, table_name);
    mag_ptr->block_ptr = CAST_PTR(uint64_t, block_ptr);
    mag_ptr->lookup_entry_num = lookup_entry_num;
    mag_ptr->backend_num = backend_num;
    mag_ptr->lookup_entry = CAST_PTR(int *, CAST_PTR(char *, block_ptr));
    mag_ptr->backend_ip = CAST_PTR(uint32_t *, CAST_PTR(char *, block_ptr) + lookup_entry_num * sizeof(int));
    mag_ptr->permutation = CAST_PTR(uint32_t *, CAST_PTR(char *, block_ptr) + lookup_entry_num * sizeof(int) + backend_num * sizeof(uint32_t));
    mag_ptr->backend_status = CAST_PTR(uint8_t *, CAST_PTR(char *, block_ptr) + lookup_entry_num * sizeof(int) + backend_num * sizeof(uint32_t) + backend_num * lookup_entry_num * sizeof(uint32_t));
    mag_ptr->next = CAST_PTR(uint32_t *, CAST_PTR(char *, block_ptr) + lookup_entry_num * sizeof(int) + backend_num * sizeof(uint32_t) + backend_num * lookup_entry_num * sizeof(uint32_t) + backend_num * sizeof(uint8_t));

    if(backend_ip == NULL || backend_status == NULL)
        return 1;
   
    maglev_update_backend(mag_ptr, backend_ip, backend_status);
    maglev_populate_lookup_entry(mag_ptr);

    return 1;
}

int maglev_destory_inner(maglev_meta_t *mag_ptr)
{
#ifdef CAVIUM_PLATFORM
    zfree(mag_ptr->table_name);
    return 1;
#else
    zfree(CAST_PTR(void *, mag_ptr->block_ptr));
    return 1;
#endif
}

// update backend_ip and backend_status;
// re-calculate permutation table;
void maglev_update_backend(maglev_meta_t *mag_ptr, uint32_t *backend_ip, uint8_t *backend_status)
{
    uint32_t offset, skip, i, j;
    uint32_t N = mag_ptr->backend_num;
    uint32_t M = mag_ptr->lookup_entry_num;

    memcpy(mag_ptr->backend_ip, backend_ip, sizeof(uint32_t) * N);
    memcpy(mag_ptr->backend_status, backend_status, sizeof(uint8_t) * N);
    
    for(i = 0; i < N; i++)
    {
        if(backend_status[i] != HEALTHY)
            continue;
        
        offset = (uint32_t)fnv_64a_buf((void *)&backend_ip[i], sizeof(uint32_t), FNV1A_64_INIT);
        skip = (uint32_t)fnv_64a_buf((void *)&backend_ip[i], sizeof(uint32_t), FNV1A_64_INIT);
        offset %= M;
        skip = skip % (M-1) + 1;
        for(j = 0; j < M; j++)
        {
            mag_ptr->permutation[i * M + j] = (offset + j * skip) % M;
        }
    }
}

// populate the lookup_entry table
void maglev_populate_lookup_entry(maglev_meta_t *mag_ptr)
{
    uint32_t N = mag_ptr->backend_num;
    uint32_t M = mag_ptr->lookup_entry_num;
    uint32_t *next = mag_ptr->next;
    int *entry = mag_ptr->lookup_entry;
    uint32_t *permutation = mag_ptr->permutation;
    uint8_t *backend_status = mag_ptr->backend_status;
    uint32_t n, c, i;

    memset(next, 0, sizeof(uint32_t) * N);
    memset(entry, -1, sizeof(int) * M);

    n = 0;
    while(1)
    {
        for(i = 0; i < N; i++)
        {
            if(backend_status[i] != HEALTHY)
                continue;
            c = permutation[i * M + next[i]];
            while(entry[c] >= 0)
            {
                next[i] ++;
                c = permutation[i * M + next[i]];
            }
            entry[c] = i;
            next[i] ++;
            n ++;
            if(n == M)
                return;
        }
    }
}

uint32_t maglev_get_backend(maglev_meta_t *mag_ptr, five_tuple_t key)
{
    uint32_t hash_value, backend_index;

    hash_value = (uint32_t)fnv_64a_buf((void *)&key, sizeof(key), FNV1A_64_INIT);
    // MurmurHash3_x86_32((void *)&key, sizeof(key), CMS_SH1, &hash_value);
    // hash_value = *(uint32_t*)&key;
    hash_value %= mag_ptr->lookup_entry_num;
    backend_index = CAST_PTR(uint32_t, mag_ptr->lookup_entry[hash_value]);

    return mag_ptr->backend_ip[backend_index];
}


#ifdef	__cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif /* __MAGLEV_HASH_H__ */
