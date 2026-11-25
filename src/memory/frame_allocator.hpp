#pragma once

// Frame Allocator Interface

#include "common.hpp"

namespace memory {
namespace frame_allocator {

void init(paddr_t physical_memory_offset, paddr_t memory_size);
paddr_t allocate_frame();
void deallocate_frame(paddr_t frame);
usize free_frame_count();
usize used_frame_count();

} // namespace frame_allocator
} // namespace memory