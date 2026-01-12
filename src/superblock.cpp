#include "superblock.hpp"

#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <stdexcept>

static std::size_t round_up_to_page(std::size_t n) {
    const long page = ::sysconf(_SC_PAGESIZE);
    if (page <= 0) return n;
    const std::size_t p = static_cast<std::size_t>(page);
    return (n + (p - 1)) & ~(p - 1);
}

Superblock::Superblock(std::size_t bytes) {
    if (bytes == 0) {
        throw std::invalid_argument("Superblock size must be > 0");
    }

    size_ = round_up_to_page(bytes);

    void* p = ::mmap(
        nullptr,
        size_,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0
    );

    if (p == MAP_FAILED) {
        const int err = errno;
        throw std::runtime_error(
            std::string("mmap failed: ") + std::strerror(err)
        );
    }

    base_ = p;
}

Superblock::Superblock(Superblock&& other) noexcept
    : base_(other.base_), size_(other.size_) {
    other.base_ = nullptr;
    other.size_ = 0;
}

Superblock& Superblock::operator=(Superblock&& other) noexcept {
    if (this != &other) {
        release();
        base_ = other.base_;
        size_ = other.size_;
        other.base_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

Superblock::~Superblock() {
    release();
}

void Superblock::release() noexcept {
    if (base_ && size_ > 0) {
        ::munmap(base_, size_);
        base_ = nullptr;
        size_ = 0;
    }
}

std::pair<MemoryRegion, MemoryRegion> Superblock::split_half() const noexcept {
    const std::size_t half = size_ / 2;

    char* base = static_cast<char*>(base_);
    MemoryRegion lo{ base, half };
    MemoryRegion hi{ base + half, size_ - half };

    return { lo, hi };
}
