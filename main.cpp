#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096

size_t ensureFullPage (size_t size, size_t page_size)
{   
    // create last 12 bits mask
    size_t offset = ~(page_size - 1);
    return (size + page_size - 1) & offset;
}

void *myMalloc (size_t size)
{
    size_t allocatedSize = ensureFullPage (size, PAGE_SIZE);
    std::cout << "allocating " << allocatedSize << " bytes" << std::endl;
    void *addrPtr = mmap(NULL, allocatedSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (addrPtr == MAP_FAILED)
    {
        perror("mmap failed");
        return nullptr;
    }
    return addrPtr;
}

void myFree(void *addrPtr, size_t size)
{
    size = ensureFullPage(size, PAGE_SIZE);
    if (addrPtr)
    {
        munmap(addrPtr, size);
    }
}

int main ()
{
    // size in bytes
    int size = 100;
    void *ptr = myMalloc(size);

    if (ptr) {
        std::cout << "allocated: " << ptr << std::endl;

        // Write something
        std::strcpy(static_cast<char*>(ptr), "Hello");
        std::cout << "wrote: " << static_cast<char*>(ptr) << std::endl;

        myFree(ptr, size);
        std::cout << "freed." << std::endl;
    }

    return 1;
}