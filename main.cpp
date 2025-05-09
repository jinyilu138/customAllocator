#include <sys/mman.h>
#include <iostream>

#include "freelist/freelist.hpp"
#include "buddy/buddy.hpp"

void* superBlockBase = nullptr;
char* freeListEnd = nullptr;
char* buddyAllocStart = nullptr;
constexpr size_t SUPERBLOCK_SIZE = 1 << 20; // 1MB


void initSuperblock() {

    superBlockBase = mmap(NULL, SUPERBLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (superBlockBase == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    freeListEnd = static_cast<char*>(superBlockBase);                      // grows up
    buddyAllocStart = static_cast<char*>(superBlockBase) + SUPERBLOCK_SIZE; // grows down
}

void cleanup() {
    if (superBlockBase) {
        munmap(superBlockBase, SUPERBLOCK_SIZE);
        superBlockBase = nullptr;
    }
}


int main() {

    initSuperblock();

    // -------- Free List Allocator Test --------
    std::cout << "\n[Free List Allocator Test]" << std::endl;
    initFreeList((1 << 19)); // 512KB for free list

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
    initBuddyAllocation(1 << 19); // 512KB for buddy (must be power of 2)

    printBuddyList();

    void* x = buddyMalloc(64);
    void* y = buddyMalloc(200);
    std::cout << "Allocated buddy blocks x=" << x << ", y=" << y << "\n";

    printBuddyList();

    buddyFree(x);
    buddyFree(y);
    std::cout << "After freeing blocks:" << std::endl;
    printBuddyList();



    // return memory to system
    cleanup();
}
