#include <iostream>

#include "freelist.hpp"
#include "buddy.hpp"
#include "superblock.hpp"

int main() {

    Superblock sb(1 << 20); // 1MB
    auto [freeR, buddyR] = sb.split_half();

    // initialize allocators (Stage 2 requirement)
    initFreeList(freeR);
    initBuddyAllocation(buddyR);

    // -------- Free List Allocator Test --------
    std::cout << "\n[Free List Allocator Test]" << std::endl;

    printFreeList();
    void* a = freeListMalloc(100);
    void* b = freeListMalloc(200);
    std::cout << "Allocated free list blocks a=" << a << ", b=" << b << "\n";

    freeListFree(a);
    freeListFree(b);
    std::cout << "After freeing blocks:" << std::endl;
    printFreeList();

    // -------- Buddy Allocator Test --------
    std::cout << "\n[Buddy Allocator Test]" << std::endl;

    printBuddyList();

    void* x = buddyMalloc(64);
    void* y = buddyMalloc(200);
    std::cout << "Allocated buddy blocks x=" << x << ", y=" << y << "\n";

    printBuddyList();

    buddyFree(x);
    buddyFree(y);
    std::cout << "After freeing blocks:" << std::endl;
    printBuddyList();
    return 0;
}
