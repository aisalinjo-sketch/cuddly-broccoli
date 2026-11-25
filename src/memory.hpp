#pragma once

// Memory Management Interface

#include "common.hpp"

namespace memory {

// Initialize memory subsystem
void init(paddr_t physical_memory_offset, paddr_t memory_size);

// Physical frame allocation
paddr_t allocate_frame();
void deallocate_frame(paddr_t frame);
usize free_frame_count();
usize used_frame_count();

// Kernel heap allocation
void* kmalloc(usize size, usize align = 16);
void kfree(void* ptr);
void* krealloc(void* ptr, usize new_size);

// Kernel stack allocation
paddr_t kernel_stack_alloc();
void kernel_stack_free(paddr_t stack);

// Memory statistics
struct MemoryStats {
    usize total_frames;
    usize free_frames;
    usize used_frames;
    usize heap_used;
    usize heap_free;
};

MemoryStats get_stats();

} // namespace memory