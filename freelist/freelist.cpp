#include "freelist.hpp"
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define MIN_SPLIT_SIZE 16

// allocate memory in blocks, need a header for each block
// block size = header, payload and padding
// payload is the allocated size
struct block {
    size_t payload;
    bool free;
    block *next;
    block *prev;
};

extern char* freeListEnd; // include from superblock
block *freeList = nullptr;

// wrapper to get super block
void initFreeList (size_t size)
{
    freeList = reinterpret_cast<block*>(freeListEnd);

    // init the block
    freeList->payload = size - sizeof(block);
    freeList->free = true;
    freeList->next = nullptr;
    freeList->prev = nullptr;
}

// free from superblock
void freeListFree(void *ptr)
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

// allocate from superblock
void *freeListMalloc (size_t size)
{
    if (freeList == nullptr)
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
