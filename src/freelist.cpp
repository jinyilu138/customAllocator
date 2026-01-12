#include "freelist.hpp"
#include "memory_region.hpp"
#include <iostream>
#include <unistd.h>

constexpr std::size_t MIN_SPLIT_SIZE = 16;

// allocate memory in blocks, need a header for each block
// block size = header, payload and padding
// payload is the allocated size
struct block {
    size_t payload;
    bool free;
    block *next;
    block *prev;
};

static char*  g_base = nullptr;
static size_t g_size = 0;
static block* g_head = nullptr;

static bool inRange(const void* p) {
    auto addr = reinterpret_cast<const char*>(p);
    return g_base && addr >= g_base && addr < (g_base + g_size);
}

// wrapper to get super block
void initFreeList (MemoryRegion region)
{
    g_base = static_cast<char*>(region.base);
    g_size = region.size;

    g_head = nullptr;

    if (!g_base || g_size < sizeof(block)) {
        std::cerr << "FreeList init: invalid region\n";
        g_base = nullptr; 
        g_size = 0;
        g_head = nullptr;
        return;
    }
    // init the block
    g_head = reinterpret_cast<block*>(g_base);
    g_head->payload = region.size - sizeof(block);
    g_head->free = true;
    g_head->next = nullptr;
    g_head->prev = nullptr;
}

// free from superblock
void freeListFree(void *ptr)
{
    if (!ptr) return;

    block *current = reinterpret_cast<block *>(ptr) - 1;
    if (!inRange(current)) return;
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
            if (current == g_head)
            {
                g_head = prev;
            }

        }

    }
    // check front
}

// allocate from superblock
void *freeListMalloc (size_t size)
{
    if (!g_head || size == 0) return nullptr;

    block *current = g_head;
    block *prev = nullptr;
    // first fit for now
    while (current)
    {
        if (current->free)
        {
            if ((current->payload >= size + sizeof(block) + MIN_SPLIT_SIZE) && (current->free == true))
            {
                block *newBlock = reinterpret_cast<block *>(reinterpret_cast<char *>(current + 1) + size);

                newBlock->payload = current->payload - size - sizeof(block);
                newBlock->free = true;
                newBlock->next = current->next;
                newBlock->prev = current;

                if (current->next) current->next->prev = newBlock;

                current->payload = size;
                current->free = false;
                current->next = newBlock;

                // skip size go to payload
                return reinterpret_cast<void *>(current + 1);
            }
            else if ((current->payload >= size + sizeof(block)) && (current->free == true))
            {
                // just give whole block
                current->free = false;
                return reinterpret_cast<void *>(current + 1);
            }

        }
        // splitting the superblock.. need to include size of header 
        // and leave at least more than an annoying amount of memory
        current = current->next;
    }
    return nullptr;
}

void printFreeList()
{
    std::cout << "---- Free List ----" << std::endl;
    block *current = g_head;
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
