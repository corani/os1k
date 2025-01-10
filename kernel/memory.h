#pragma once

#include "../common.h"

#define PAGE_V    (1 << 0)  // Valid (entry is enabled)
#define PAGE_R    (1 << 1)  // Readable
#define PAGE_W    (1 << 2)  // Writable
#define PAGE_X    (1 << 3)  // Executable
#define PAGE_U    (1 << 4)  // User (accessible in user mode)
                            //
paddr_t alloc_pages(uint32_t n);
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);
