#ifndef __VIRT_TO_PHYS_H__
#define __VIRT_TO_PHYS_H__

// https://stackoverflow.com/questions/2440385/how-to-find-the-physical-address-of-a-variable-from-user-space-in-linux

#include "stdio.h"
#include "unistd.h"
#include "inttypes.h"
#include<sys/mman.h>

// TODO: we need to pin the packet memory
uintptr_t virt_to_phys_user(uintptr_t vaddr) {
    FILE *pagemap;
    intptr_t paddr = 0;
    int offset = (vaddr / sysconf(_SC_PAGESIZE)) * sizeof(uint64_t);
    uint64_t e;

    // https://www.kernel.org/doc/Documentation/vm/pagemap.txt
    if ((pagemap = fopen("/proc/self/pagemap", "r"))) {
        if (lseek(fileno(pagemap), offset, SEEK_SET) == offset) {
            if (fread(&e, sizeof(uint64_t), 1, pagemap)) {
                if (e & (1ULL << 63)) { // page present ?
                    paddr = e & ((1ULL << 54) - 1); // pfn mask
                    paddr = paddr * sysconf(_SC_PAGESIZE);
                    // add offset within page
                    paddr = paddr | (vaddr & (sysconf(_SC_PAGESIZE) - 1));
                }   
            }   
        }   
        fclose(pagemap);
    }   

    return paddr;
}   

void pin_process_memory() {
    if(mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
        perror("mlockall");
        exit(0);
    }
}

#endif // __VIRT_TO_PHYS_H__