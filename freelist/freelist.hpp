#ifndef FREELIST_ALLOCATOR_HPP
#define FREELIST_ALLOCATOR_HPP

#include <cstddef>  // for size_t

void  initFreeList(size_t size);
void* freeListMalloc(size_t size);
void  freeListFree(void* ptr);
void  printFreeList();

#endif // FREELIST_ALLOCATOR_HPP
