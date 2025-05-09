// what is buddy algo
// split by powers of 2
// mark buddies so that we know how to coalese again 
// when sizes are not a power of 2 we have to allocate the entire "buddy block"
// this lease to leftover memory -> internal fragmentation (wasted space :()
// kinda looks like a binary tree, keep splitting the next largest block
// can coalese only when both buddies are free (ie both subranches are free)
#include "buddy.hpp"
#include <sys/mman.h>
#include <iostream>
#include <cmath>


constexpr int MIN_ORDER = 5;  // 2^5 = 32 bytes
constexpr int MAX_ORDER = 19; // 2^20 = 1 MB
constexpr size_t TOTAL_BUDDY_HEAP = 1 << 19;

struct block {
    int order; // keep track of size via orders of 2
    bool free;
    block *next;
};

int getOrder(size_t size);
block* getSplitBlock(int order);


extern char* buddyAllocStart;
block *buddyTracker = nullptr;
block* buddyFreeLists[MAX_ORDER + 1] = {nullptr};

// init the block allocated for buddy allocator
// allocator will grow leftwards/downwards/backwards
void  initBuddyAllocation(size_t size)
{
    if (size != (1 << MAX_ORDER))
    {
        std::cerr << "must use the 2^" << MAX_ORDER << " allocated from system" << std::endl;
        return;
    }

    // need something to keep track of the the "front" of the block
    //  ----------------------------
    // | freelist |                |
    //  ----------------------------
    //             ^buddytracker   ^buddyallocstart
    //             <----2^size---->
    char* topBlock = buddyAllocStart - size;
    // create a block of max size with nothing split
    block* freeList = reinterpret_cast<block*>(topBlock);
    freeList->order = MAX_ORDER;
    freeList->free = true;
    freeList->next = nullptr;

    buddyFreeLists[MAX_ORDER] = freeList;
    buddyTracker = freeList;
    buddyAllocStart = topBlock;
}

void* buddyMalloc(size_t size)
{
    // map requested size to one that fits within one of the blocks
    int order = getOrder(size);
    //get to smallest suitable block and keep splitting if possibel
    block* buddy = getSplitBlock(order);
    if (!buddy)
    {
        return nullptr;
    }
    //+1 is 1 move by size of buddy's type
    return reinterpret_cast<void*>(buddy+1);
}

void buddyFree(void* ptr) {
    if (!ptr) return;

    block* current = reinterpret_cast<block*>(ptr) - 1;
    current->free = true;

    // coalesce to the right/up
    while (current->order < MAX_ORDER) {
        size_t blockSize = 1UL << current->order;

        uintptr_t currentOffset = reinterpret_cast<uintptr_t>(current) - reinterpret_cast<uintptr_t>(buddyTracker);

        uintptr_t buddyOffset = currentOffset ^ blockSize;
        block* buddy = reinterpret_cast<block*>(reinterpret_cast<char*>(buddyTracker) + buddyOffset);

        if (!(buddy->free && buddy->order == current->order)) {
            break; // can't coalesce
        }

        block** list = &buddyFreeLists[buddy->order];
        while (*list && *list != buddy) {
            list = &((*list)->next);
        }
        if (*list == buddy) {
            *list = buddy->next;
        }

        if (buddy < current) {
            std::swap(buddy, current);
        }

        // merge blocks
        current->order += 1;
        current->free = true;
        current->next = nullptr;
    }

    // insert final merged block into the free list
    current->next = buddyFreeLists[current->order];
    buddyFreeLists[current->order] = current;
}

void printBuddyList() {
    std::cout << "===== Buddy Free Lists =====" << std::endl;
    for (int order = MIN_ORDER; order <= MAX_ORDER; ++order) {
        std::cout << "Order " << order << " (Block size: " << (1 << order) << "): ";

        block* current = buddyFreeLists[order];
        if (!current) {
            std::cout << "[empty]";
        } else {
            while (current) {
                std::cout << "[" << current << "]";
                current = current->next;
                if (current) std::cout << " -> ";
            }
        }

        std::cout << std::endl;
    }
    std::cout << "=============================" << std::endl;
}


int getOrder(size_t size)
{
    // get total size
    size_t sizeTotal = size + sizeof(block);
    int order = MIN_ORDER;
    while (((1 << order) < sizeTotal) && order < MAX_ORDER)
    {
        order++;
    }
    return order;
}

block* getSplitBlock(int order)
{
    int orderCount = order;
    // find biggest block closest to 2^order size
    while (orderCount <= MAX_ORDER && buddyFreeLists[orderCount] == nullptr)
    {
        orderCount++;
    }
    if (orderCount > MAX_ORDER)
    {
        std::cerr << "No available blocks in buddy allocator\n";
        return nullptr;
    }

    block* splitBlock = buddyFreeLists[orderCount];
    buddyFreeLists[orderCount] = splitBlock->next;

    while (orderCount > order)
    {
        --orderCount;
        size_t size = 1 << orderCount;

        char* buddyAddr = reinterpret_cast<char*>(splitBlock) + size;
        block* buddy = reinterpret_cast<block*>(buddyAddr);

        buddy->order = orderCount;
        buddy->free = true;
        buddy->next = buddyFreeLists[orderCount];
        buddyFreeLists[orderCount] = buddy;
    }

    splitBlock->free = false;
    splitBlock->next = nullptr;

    return splitBlock;
}