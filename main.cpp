#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096

// allocate memory in blocks, need a header for each block
// block size = header, payload and padding
// payload is the allocated size
// plannign for doubly linked list
struct block {
    size_t payload;
    bool free;
    block *next;

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

    block *header = reinterpret_cast<block *>(ptr) - 1;
    header->free = true;
}

void *myMalloc (size_t size)
{
    if ((superBlockPtr == nullptr) || (freeList == nullptr))
    {
        perror("something failed");
        return nullptr;
    }
    block *current = freeList;
    // first fit for now
    while (current)
    {
        if ((current->payload >= size) && (current->free == true))
        {
            freeList->free = false;
            // skip size go to payload
            return reinterpret_cast<void *>(current + 1);
        }
        current = current->next;
    }
    perror("no block available");
    return nullptr;
}

int main ()
{
    const size_t superBlockSize = 1 << 20; // 1 MB
    getSuperBlock(superBlockSize);

    size_t size = 100;
    void *ptr = myMalloc(size);

    if (ptr) {
        std::cout << "allocated: " << ptr << std::endl;

        // Write something
        std::strcpy(static_cast<char*>(ptr), "Hello");
        std::cout << "wrote: " << static_cast<char*>(ptr) << std::endl;

        myFree(ptr);
        finalFree(superBlockPtr, superBlockSize);
        std::cout << "freed." << std::endl;
    }

    return 1;
}