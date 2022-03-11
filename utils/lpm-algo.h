#ifndef __LPM_ALGO_H
#define __LPM_ALGO_H

#ifdef	__cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#include <stdint.h>
#include <stdio.h>
#include "common.h" 

#define TBL24_SIZE ((1 << 24) + 1)
#define RAW_SIZE 33
#define OVERFLOW_MASK 0x8000
#define GATE_NUM 256

#define MAX_ENTRY_NUM 16000

typedef struct
{
    uint32_t ip;
    uint16_t gate;
}raw_entry_t;

typedef struct 
{
    raw_entry_t raw_entries[MAX_ENTRY_NUM];
    uint32_t cur_num;
}raw_entry_list_t;

typedef struct{
    uint16_t tbl24[TBL24_SIZE];
    uint16_t tbl_long[TBL24_SIZE];
    uint32_t current_tbl_long;
    raw_entry_list_t raw_entry_list[RAW_SIZE];
}iplookup_t;

void lpm_init_inner(iplookup_t *iplookup)
{
    uint32_t i = 0, j = 0;
    for(i = 0; i < TBL24_SIZE; i++)
    {
        iplookup->tbl24[i] = iplookup->tbl_long[i] = 0;
    }
    iplookup->current_tbl_long = 0;
    for(i = 0; i < RAW_SIZE; i++)
    {
        iplookup->raw_entry_list[i].cur_num = 0;
        for(j = 0; j < MAX_ENTRY_NUM; j++)
        {
            iplookup->raw_entry_list[i].raw_entries[j].ip = 0;
            iplookup->raw_entry_list[i].raw_entries[j].gate = 0;
        }
    }
}

void lpm_insert(iplookup_t *iplookup, uint32_t ip, uint32_t len, uint16_t gate)
{
    uint32_t cur_num = iplookup->raw_entry_list[len].cur_num++;
    iplookup->raw_entry_list[len].raw_entries[cur_num].ip = ip;
    iplookup->raw_entry_list[len].raw_entries[cur_num].gate = gate;
}

/*
    pub fn construct_table(&mut self) {
        for i in 0..25 {
            for (k, v) in &self.raw_entries[i] {
                let start = (k >> 8) as usize;
                let end = (start + (1 << (24 - i))) as usize;
                for pfx in start..end {
                    self.tbl24[pfx] = *v;
                }
            }
        }
        for i in 25..RAW_SIZE {
            for (k, v) in &self.raw_entries[i] {
                let addr = *k as usize;
                let t24entry = self.tbl24[addr >> 8];
                if (t24entry & OVERFLOW_MASK) == 0 {
                    // Not overflown and entered yet
                    let ctlb = self.current_tbl_long;
                    let start = ctlb + (addr & 0xff); // Look at last 8 bits (since first 24 are predetermined.
                    let end = start + (1 << (32 - i));
                    for j in ctlb..(ctlb + 256) {
                        if j < start || j >= end {
                            self.tbl_long[j] = t24entry;
                        } else {
                            self.tbl_long[j] = *v;
                        }
                    }
                    self.tbl24[addr >> 8] = ((ctlb >> 8) as u16) | OVERFLOW_MASK;
                    self.current_tbl_long += 256;
                } else {
                    let start = (((t24entry & (!OVERFLOW_MASK)) as usize) << 8) + (addr & 0xff);
                    let end = start + (1 << (32 - i));
                    for j in start..end {
                        self.tbl_long[j] = *v;
                    }
                }
            }
        }
    }
 */
void lpm_construct_table(iplookup_t *iplookup)
{
    uint32_t i = 0, j = 0, pfx = 0;
    uint32_t cur_num = 0;
    uint32_t k;
    uint16_t v;
    for(i = 0; i < 25; i++)
    {
        cur_num = iplookup->raw_entry_list[i].cur_num;
        for(j = 0; j < cur_num; j++)
        {
            k = iplookup->raw_entry_list[i].raw_entries[j].ip;
            v = iplookup->raw_entry_list[i].raw_entries[j].gate;
            uint32_t start = k >> 8;
            uint32_t end = start + (1 << (24 - i));
            for(pfx = start; pfx < end; pfx++)
            {
                iplookup->tbl24[pfx] = v;
            }
        }
    }
    for(i = 25; i < RAW_SIZE; i++)
    {
        cur_num = iplookup->raw_entry_list[i].cur_num;
        for(j = 0; j < cur_num; j++)
        {
            k = iplookup->raw_entry_list[i].raw_entries[j].ip;
            v = iplookup->raw_entry_list[i].raw_entries[j].gate;
            uint32_t addr = k;
            uint16_t t24entry = iplookup->tbl24[addr >> 8];
            if((t24entry & OVERFLOW_MASK) == 0)
            {
                uint32_t ctlb = iplookup->current_tbl_long;
                uint32_t start = ctlb + (addr && 0xff);
                uint32_t end = start + (1 << (32 - i));
                for(pfx = ctlb; pfx < ctlb + 256; pfx ++)
                {
                    if(pfx < start || pfx >= end)
                    {
                        iplookup->tbl_long[pfx] = t24entry;
                    }
                    else
                    {
                        iplookup->tbl_long[pfx] = v;
                    }
                }
                iplookup->tbl24[addr >> 8] = (uint16_t)(ctlb >> 8) | OVERFLOW_MASK;
                iplookup->current_tbl_long += 256;
            }
            else
            {
                uint32_t start = ((uint32_t)(t24entry & (!OVERFLOW_MASK)) << 8) + (addr & 0xFF);
                uint32_t end = start + (1 << (32 - i));
                for(pfx = start; pfx < end; pfx ++)
                {
                    iplookup->tbl_long[pfx] = v;
                }
            }
        }        
    }
}

/*
pub fn lookup_entry(&self, ip: Ipv4Addr) -> u16 {
        let addr = u32::from(ip) as usize;
        let t24entry = self.tbl24[addr >> 8];
        if (t24entry & OVERFLOW_MASK) > 0 {
            let index = (((t24entry & !OVERFLOW_MASK) as usize) << 8) + (addr & 0xff);
            self.tbl_long[index]
        } else {
            t24entry
        }
    }
*/
inline uint16_t lpm_lookup(iplookup_t *iplookup, uint32_t ip)
{
    uint32_t addr = ip;
    uint16_t t24entry = iplookup->tbl24[addr >> 8];
    if((t24entry & OVERFLOW_MASK) > 0)
    {
        uint32_t index = ((uint32_t)(t24entry & !OVERFLOW_MASK) << 8) + (addr & 0xFF);
        return iplookup->tbl_long[index];
    }
    else
    {
        return t24entry;
    }
}


#ifdef	__cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

#endif//__LPM_ALGO_H