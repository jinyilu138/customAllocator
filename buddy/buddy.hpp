#ifndef BUDDY_ALLOCATOR_HPP
#define BUDDY_ALLOCATOR_HPP

#include <cstddef>  // for size_t

void  initBuddyAllocation(size_t size);
void* buddyMalloc(size_t size);
void  buddyFree(void* ptr);
void  printBuddyList();

#endif // BUDDY_ALLOCATOR_HPP