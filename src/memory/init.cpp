// Memory Subsystem Initialization

#include "memory.hpp"
#include "arch.hpp"
#include "common.hpp"

namespace memory {

void init(paddr_t physical_memory_offset, paddr_t memory_size) {
    // Initialize frame allocator
    frame_allocator::init(physical_memory_offset, memory_size);

    // Initialize heap (1MB initial heap from linker script)
    extern char heap_start[];
    extern char heap_end[];
    init_heap(reinterpret_cast<vaddr_t>(heap_start), reinterpret_cast<vaddr_t>(heap_end));
}

} // namespace memory