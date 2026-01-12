#ifndef FREELIST_ALLOCATOR_HPP
#define FREELIST_ALLOCATOR_HPP

#include <cstddef>  // for size_t
#include "memory_region.hpp"

void  initFreeList(MemoryRegion region);
void* freeListMalloc(size_t size);
void  freeListFree(void* ptr);
void  printFreeList();

#endif // FREELIST_ALLOCATOR_HPP
