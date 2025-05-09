# 🧠 Custom Dynamic Memory Allocator

This project implements a hybrid dynamic memory allocator in C++ using two strategies: a **Free List Allocator** for small allocations and a **Buddy Allocator** for larger allocations. 
Both allocators share a single preallocated memory region and grow from opposite ends of the block.

---

## ❓ Why 

The idea of building my own allocator didn't come with the goal to perform better than malloc. I wanted to learn more about how the malloc that I always use works under the hood.
With working on this project I learned lots about my computer's underlying memory structure in addition to various other tidbits about programming in c++ :).

---

## ⚙️ Allocator Types

I chose to create a hybrid model to allow for better general use. Both designs have their respective strengths and weaknesses so a hybrid approach provides the best of both worlds.

### Free List Allocator
- maintain a linked list of variable-size memory blocks
- implemented with first-fit strategy with block splitting and coalescing
- good for small, frequent allocations
- simple and space-efficient, but suffers from fragmentation

### Buddy Allocator
- allocates memory in powers of two, imagine a binary tree
- quick splitting and merging
- ideal for embedded systems due to low fragmentation and predictability
- internal fragmentation (wasted memory within each block) will occur when requested size does not exactly fit the a 2^x block size

---

## 🔧 Build & Run
Requires clang++ or g++ with C++17 support
```bash
make
./allocator
