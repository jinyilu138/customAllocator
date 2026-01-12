// what is buddy algo
// split by powers of 2
// mark buddies so that we know how to coalese again 
// when sizes are not a power of 2 we have to allocate the entire "buddy block"
// this lease to leftover memory -> internal fragmentation (wasted space :()
// kinda looks like a binary tree, keep splitting the next largest block
// can coalese only when both buddies are free (ie both subranches are free)
#include "buddy.hpp"
#include "memory_region.hpp"
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

static char* g_base = nullptr;
static size_t g_size = 0;
static int g_max_order = 0;
block* buddyFreeLists[MAX_ORDER + 1] = {nullptr};

static int floorLog2(size_t x) {
    int r = 0;
    while ((size_t(1) << (r + 1)) <= x) r++;
    return r;
}

static int ceilLog2(size_t x) {
    int r = 0;
    size_t v = 1;
    while (v < x) { v <<= 1; r++; }
    return r;
}

static size_t blockSize(int order) {
    return size_t(1) << order;
}

static block* ptrToBlock(void* p) {
    return reinterpret_cast<block*>(p) - 1;
}

static void* blockToPtr(block* b) {
    return reinterpret_cast<void*>(b + 1);
}

static bool inRange(const void* p) {
    auto addr = reinterpret_cast<const char*>(p);
    return g_base && addr >= g_base && addr < (g_base + g_size);
}

// Buddy of a block: base + (offset XOR blockSize)
static block* buddyOf(block* b) {
    const size_t sz = blockSize(b->order);
    const uintptr_t offset = uintptr_t(reinterpret_cast<char*>(b) - g_base);
    const uintptr_t buddyOffset = offset ^ uintptr_t(sz);
    return reinterpret_cast<block*>(g_base + buddyOffset);
}

static void pushFree(int order, block* b) {
    b->order = order;
    b->free = true;
    b->next = buddyFreeLists[order];
    buddyFreeLists[order] = b;
}

static block* popFree(int order) {
    block* b = buddyFreeLists[order];
    if (!b) return nullptr;
    buddyFreeLists[order] = b->next;
    b->next = nullptr;
    return b;
}

static bool removeFromFreeList(int order, block* target) {
    block** cur = &buddyFreeLists[order];
    while (*cur) {
        if (*cur == target) {
            *cur = target->next;
            target->next = nullptr;
            return true;
        }
        cur = &((*cur)->next);
    }
    return false;
}

// init the block allocated for buddy allocator
// allocator will grow leftwards/downwards/backwards
void initBuddyAllocation(MemoryRegion region) {
    g_base = static_cast<char*>(region.base);
    g_size = region.size;

    for (int i = 0; i <= MAX_ORDER; i++) buddyFreeLists[i] = nullptr;

    if (!g_base || g_size == 0) {
        std::cerr << "Buddy init: invalid region\n";
        g_base = nullptr; g_size = 0; g_max_order = 0;
        return;
    }

    // Compute max order from region size, but clamp to MAX_ORDER for our freelist array.
    g_max_order = floorLog2(g_size);
    if (g_max_order > MAX_ORDER) g_max_order = MAX_ORDER;

    // Require region to be at least one block at MIN_ORDER and to fit the header.
    if (g_max_order < MIN_ORDER || blockSize(g_max_order) < sizeof(block)) {
        std::cerr << "Buddy init: region too small\n";
        g_base = nullptr; g_size = 0; g_max_order = 0;
        return;
    }

    // Use largest power-of-two chunk <= region.size
    const size_t usable = blockSize(g_max_order);

    block* root = reinterpret_cast<block*>(g_base);
    root->order = g_max_order;
    root->free = true;
    root->next = nullptr;

    buddyFreeLists[g_max_order] = root;

    // Optional: ignore the tail [g_base+usable, g_base+g_size) for now
}

void buddyFree(void* ptr) {
    if (!ptr) return;

    block* current = ptrToBlock(ptr);
    if (!inRange(current)) return;
    current->free = true;

    // coalesce to the right/up
    while (current->order < g_max_order) {
        block* bud = buddyOf(current);

        // bud must also be in range; otherwise stop
        if (!inRange(bud)) break;

        if (!(bud->free && bud->order == current->order)) break;

        // Remove buddy from free list before merging
        if (!removeFromFreeList(bud->order, bud)) break;

        // Merge base is the lower address
        current = (bud < current) ? bud : current;
        current->order += 1;
        current->free = true;
        current->next = nullptr;
    }

    pushFree(current->order, current);
}

void printBuddyList() {
    std::cout << "===== Buddy Free Lists =====" << std::endl;
    for (int order = MIN_ORDER; order <= g_max_order; ++order) {
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


static int getOrder(size_t size)
{
    const size_t total = size + sizeof(block);
    int order = ceilLog2(total);
    if (order < MIN_ORDER) order = MIN_ORDER;
    return order;
}

static block* getSplitBlock(int order)
{
    int orderCount = order;
    // find biggest block closest to 2^order size
    while (orderCount <= g_max_order && buddyFreeLists[orderCount] == nullptr)
    {
        orderCount++;
    }
    if (orderCount > g_max_order)
    {
        std::cerr << "No available blocks in buddy allocator\n";
        return nullptr;
    }

    block* splitBlock = popFree(orderCount);
    if (!splitBlock)
    {
        std::cerr << "Error: expected non-null block in free list\n";
        return nullptr;
    }
    while (orderCount > order)
    {
        --orderCount;
        splitBlock->order = orderCount;
        splitBlock->free = true;

        // Create buddy half at +2^o
        char* buddyAddr = reinterpret_cast<char*>(splitBlock) + blockSize(orderCount);
        block* bud = reinterpret_cast<block*>(buddyAddr);
        bud->order = orderCount;
        bud->free = true;
        bud->next = nullptr;

        // Put buddy half into free list
        pushFree(orderCount, bud);
    }

    splitBlock->order = order;
    splitBlock->free = false;
    splitBlock->next = nullptr;
    return splitBlock;
}

void* buddyMalloc(size_t size)
{
    if(!g_base) return nullptr;

    // map requested size to one that fits within one of the blocks
    int order = getOrder(size);
    if (order > g_max_order) return nullptr;
    //get to smallest suitable block and keep splitting if possibel
    block* buddy = getSplitBlock(order);
    if (!buddy)
    {
        return nullptr;
    }
    //+1 is 1 move by size of buddy's type
    return blockToPtr(buddy);
}