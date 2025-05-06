#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define MIN_SPLIT_SIZE 16  // arbutrariy

// allocate memory in blocks, need a header for each block
// block size = header, payload and padding
// payload is the allocated size
struct block {
    size_t payload;
    bool free;
    block *next;
    block *prev;
};

void *superBlockPtr = nullptr;
block *freeList = nullptr;

size_t ensureFullPage (size_t size, size_t page_size)
{   
    // create last 12 bits mask
    size_t offset = ~(page_size - 1);
    return (size + page_size - 1) & offset;
}

// core memory getting
void *getMemory (size_t size)
{
    std::cout << "allocating " << size << " bytes" << std::endl;
    void *addrPtr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (addrPtr == MAP_FAILED)
    {
        perror("mmap failed");
        return nullptr;
    }
    return addrPtr;
}

// wrapper to get super block
void getSuperBlock (size_t size)
{
    size = ensureFullPage (size, PAGE_SIZE);
    superBlockPtr = getMemory(size);
    // init the block
    freeList = reinterpret_cast<block*>(superBlockPtr);
    freeList->payload = size - sizeof(block);
    freeList->free = true;
    freeList->next = nullptr;
    freeList->prev = nullptr;
}

void finalFree(void *addrPtr, size_t size)
{
    if (addrPtr)
    {
        munmap(addrPtr, size);
    }
}

void myFree(void *ptr)
{
    if (!ptr) return;

    block *current = reinterpret_cast<block *>(ptr) - 1;
    current->free = true;
    // coalese

    // check front
    block* next = current->next;
    if(next && next->free)
    {
        char *current_end = reinterpret_cast<char*> (current) + sizeof(block) + current->payload;
        if (reinterpret_cast<char*>(next) == current_end)
        {
            // increase current payload to include size of next block
            current->payload += sizeof(block) + next->payload;
            current->next = next->next;

            // ensure skip current
            if (next->next)
            {
                next->next->prev = current;
            }

        }
    }
    // check back
    block* prev = current->prev;
    // if block exists + is free
    if (prev && prev->free)
    {
        // if the current (freelist pointer) is the same as end of prev block
        // end is the pointer itself (staring offset) + the size of header, + size of the block
        char *prev_end = reinterpret_cast<char*> (prev) + sizeof(block) + prev->payload;
        if (reinterpret_cast<char*> (current) == prev_end)
        {
            // add the free list to the prev
            prev->payload += sizeof(block) + current->payload;
            prev->next = current->next;
            
            if (current->next)
            {
                current->next->prev = prev;
            }
            if (current == freeList)
            {
                freeList = prev;
            }

        }

    }
    // check front
}

void *myMalloc (size_t size)
{
    if ((superBlockPtr == nullptr) || (freeList == nullptr))
    {
        perror("something failed");
        return nullptr;
    }
    block *allocateBlock = freeList;
    block *prev = nullptr;
    // first fit for now
    while (allocateBlock)
    {
        // splitting the superblock.. need to include size of header 
        // and leave at least more than an annoying amount of memory
        if ((allocateBlock->payload >= size + sizeof(block) + MIN_SPLIT_SIZE) && (allocateBlock->free == true))
        {
            block *newSuperBlock = reinterpret_cast<block *>(reinterpret_cast<char *>(allocateBlock + 1) + size);

            newSuperBlock->payload = allocateBlock->payload - size - sizeof(block);
            newSuperBlock->free = true;
            newSuperBlock->next = allocateBlock->next;
            newSuperBlock->prev = allocateBlock;

            if (allocateBlock->next)
                allocateBlock->next->prev = newSuperBlock;

            allocateBlock->payload = size;
            allocateBlock->free = false;
            allocateBlock->next = newSuperBlock;

            // skip size go to payload
            return reinterpret_cast<void *>(allocateBlock + 1);
        }
        else if ((allocateBlock->payload >= size + sizeof(block)) && (allocateBlock->free == true))
        {
            // just give whole block
            allocateBlock->free = false;
            return reinterpret_cast<void *>(allocateBlock + 1);
        }
        prev = allocateBlock;
        allocateBlock = allocateBlock->next;
    }
    perror("no block available");
    return nullptr;
}

void printFreeList()
{
    std::cout << "---- Free List ----" << std::endl;
    block *current = freeList;
    int index = 0;
    while (current)
    {
        std::cout << "[" << index << "] "
                  << "Address: " << current
                  << " | Payload: " << current->payload
                  << " | Free: " << (current->free ? "Yes" : "No")
                  << " | Next: " << current->next
                  << " | Prev: " << current->prev
                  << std::endl;
        current = current->next;
        index++;
    }
    std::cout << "-------------------" << std::endl;
}

int main ()
{
    const size_t superBlockSize = 1 << 20; // 1 MB
    getSuperBlock(superBlockSize);

    std::cout << "Initial free list:" << std::endl;
    printFreeList();

    void *a = myMalloc(100);
    void *b = myMalloc(200);
    void *c = myMalloc(300);

    std::cout << "\nAfter allocations:" << std::endl;
    printFreeList();

        // --- Test forward coalescing (B + C) ---
    myFree(c); // Free C first (right)
    std::cout << "\nAfter freeing c (300 bytes):" << std::endl;
    printFreeList();

    myFree(b); // Free B next (left of C) → B + C coalesce
    std::cout << "\nAfter freeing b (200 bytes):" << std::endl;
    printFreeList();

    // --- Test backward coalescing (A + (B+C)) ---
    myFree(a); // Now free A → full coalesce A + B + C
    std::cout << "\nAfter freeing a (100 bytes):" << std::endl;
    printFreeList();

    finalFree(superBlockPtr, superBlockSize);


    return 1;
}