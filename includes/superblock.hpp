#pragma once

#include <cstddef>
#include <utility>
#include "memory_region.hpp"

// Owns one mmap'ed contiguous block and splits it into sub-regions.
// This is the ONLY place that should call mmap/munmap.
// Allocators (buddy/freelist/arena) should only accept MemoryRegion.
class Superblock {
public:
    explicit Superblock(std::size_t bytes);

    // Non-copyable (owns memory)
    Superblock(const Superblock&) = delete;
    Superblock& operator=(const Superblock&) = delete;

    // Movable (optional, but convenient)
    Superblock(Superblock&& other) noexcept;
    Superblock& operator=(Superblock&& other) noexcept;

    ~Superblock();

    // Entire mapped region
    MemoryRegion region() const noexcept { return { base_, size_ }; }

    // Split into two halves:
    // - first: lower half [base, base+half)
    // - second: upper half [base+half, base+size)
    //
    // Use this for: freelist gets lower half, buddy gets upper half.
    std::pair<MemoryRegion, MemoryRegion> split_half() const noexcept;

    std::size_t size() const noexcept { return size_; }
    void* base() const noexcept { return base_; }

private:
    void* base_ = nullptr;
    std::size_t size_ = 0;

    void release() noexcept; // munmap if owned
};
