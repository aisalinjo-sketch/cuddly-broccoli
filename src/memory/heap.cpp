// Kernel Heap Allocator (Simple bump allocator with free list)

#include "memory.hpp"
#include "arch.hpp"
#include "common.hpp"

namespace memory {

// Heap block header
struct alignas(16) BlockHeader {
    usize size;          // Size of usable data (not including header)
    bool free;           // Whether block is free
    BlockHeader* next;   // Next block in list
    BlockHeader* prev;   // Previous block in list
    u32 magic;           // Magic number for validation
};

constexpr u32 BLOCK_MAGIC = 0x4B484541;  // "HEAK"
constexpr usize MIN_BLOCK_SIZE = 32;
constexpr usize HEADER_SIZE = sizeof(BlockHeader);

static vaddr_t g_heap_start = 0;
static vaddr_t g_heap_end = 0;
static vaddr_t g_heap_current = 0;
static BlockHeader* g_first_block = nullptr;
static SpinLock g_heap_lock;
static usize g_heap_used = 0;

void init_heap(vaddr_t start, vaddr_t end) {
    g_heap_start = PAGE_ALIGN_UP(start);
    g_heap_end = PAGE_ALIGN_DOWN(end);
    g_heap_current = g_heap_start;
    g_first_block = nullptr;
    g_heap_used = 0;
}

void* kmalloc(usize size, usize align) {
    if (size == 0) return nullptr;

    // Align size
    size = (size + 15) & ~15;

    spinlock_guard(g_heap_lock);

    // Try to find a free block
    BlockHeader* best = nullptr;
    usize best_size = 0;

    for (BlockHeader* block = g_first_block; block; block = block->next) {
        if (block->free && block->size >= size) {
            if (!best || block->size < best_size) {
                best = block;
                best_size = block->size;
                if (block->size == size) break;  // Perfect fit
            }
        }
    }

    if (best) {
        // Split block if it's significantly larger
        if (best->size >= size + HEADER_SIZE + MIN_BLOCK_SIZE) {
            BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                reinterpret_cast<u8*>(best) + HEADER_SIZE + size
            );
            new_block->size = best->size - size - HEADER_SIZE;
            new_block->free = true;
            new_block->next = best->next;
            new_block->prev = best;
            new_block->magic = BLOCK_MAGIC;

            if (best->next) {
                best->next->prev = new_block;
            }
            best->next = new_block;
            best->size = size;
        }

        best->free = false;
        g_heap_used += best->size;
        return reinterpret_cast<u8*>(best) + HEADER_SIZE;
    }

    // No suitable block, extend heap
    vaddr_t aligned_current = (g_heap_current + align - 1) & ~(align - 1);
    usize total_size = HEADER_SIZE + size;

    if (aligned_current + total_size > g_heap_end) {
        // Need to map more pages
        usize pages_needed = (aligned_current + total_size - g_heap_end + PAGE_SIZE - 1) / PAGE_SIZE;
        for (usize i = 0; i < pages_needed; i++) {
            paddr_t frame = allocate_frame();
            if (!frame) return nullptr;
            if (!arch::paging::map_page(g_heap_end + i * PAGE_SIZE, frame, arch::paging::KERNEL_PAGE_FLAGS)) {
                return nullptr;
            }
        }
        g_heap_end += pages_needed * PAGE_SIZE;
    }

    BlockHeader* block = reinterpret_cast<BlockHeader*>(aligned_current);
    block->size = size;
    block->free = false;
    block->next = nullptr;
    block->prev = nullptr;
    block->magic = BLOCK_MAGIC;

    if (g_first_block) {
        // Find last block
        BlockHeader* last = g_first_block;
        while (last->next) last = last->next;
        last->next = block;
        block->prev = last;
    } else {
        g_first_block = block;
    }

    g_heap_current = aligned_current + total_size;
    g_heap_used += size;

    return reinterpret_cast<u8*>(block) + HEADER_SIZE;
}

void kfree(void* ptr) {
    if (!ptr) return;

    spinlock_guard(g_heap_lock);

    BlockHeader* block = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(ptr) - HEADER_SIZE);

    // Validate magic
    if (block->magic != BLOCK_MAGIC) {
        // Invalid pointer
        return;
    }

    if (block->free) {
        // Double free
        return;
    }

    block->free = true;
    g_heap_used -= block->size;

    // Coalesce with next block if free
    if (block->next && block->next->free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Coalesce with previous block if free
    if (block->prev && block->prev->free) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

void* krealloc(void* ptr, usize new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return nullptr;
    }

    BlockHeader* block = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(ptr) - HEADER_SIZE);
    if (block->magic != BLOCK_MAGIC) return nullptr;

    if (block->size >= new_size) {
        // Shrink - potentially split
        if (block->size >= new_size + HEADER_SIZE + MIN_BLOCK_SIZE) {
            spinlock_guard(g_heap_lock);
            BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                reinterpret_cast<u8*>(block) + HEADER_SIZE + new_size
            );
            new_block->size = block->size - new_size - HEADER_SIZE;
            new_block->free = true;
            new_block->next = block->next;
            new_block->prev = block;
            new_block->magic = BLOCK_MAGIC;

            if (block->next) {
                block->next->prev = new_block;
            }
            block->next = new_block;
            block->size = new_size;
        }
        return ptr;
    }

    // Need to grow - allocate new and copy
    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) return nullptr;

    __builtin_memcpy(new_ptr, ptr, block->size);
    kfree(ptr);
    return new_ptr;
}

paddr_t kernel_stack_alloc() {
    // Allocate 16KB stack (4 pages)
    constexpr usize STACK_PAGES = 4;
    paddr_t frames[STACK_PAGES];

    for (usize i = 0; i < STACK_PAGES; i++) {
        frames[i] = allocate_frame();
        if (!frames[i]) {
            // Rollback
            for (usize j = 0; j < i; j++) {
                deallocate_frame(frames[j]);
            }
            return 0;
        }
    }

    // Map to virtual address (find free virtual space)
    // For simplicity, use a fixed region after heap
    static vaddr_t next_stack_virt = 0xFFFF800010000000ULL;

    vaddr_t virt = next_stack_virt;
    next_stack_virt += STACK_PAGES * PAGE_SIZE;

    for (usize i = 0; i < STACK_PAGES; i++) {
        if (!arch::paging::map_page(virt + i * PAGE_SIZE, frames[i], arch::paging::KERNEL_PAGE_FLAGS)) {
            // Rollback
            for (usize j = 0; j <= i; j++) {
                arch::paging::unmap_page(virt + j * PAGE_SIZE);
                deallocate_frame(frames[j]);
            }
            return 0;
        }
    }

    // Return top of stack
    return virt + STACK_PAGES * PAGE_SIZE;
}

void kernel_stack_free(paddr_t stack_top) {
    // Unmap and deallocate 4 pages
    vaddr_t virt = stack_top - 4 * PAGE_SIZE;
    for (usize i = 0; i < 4; i++) {
        paddr_t phys = arch::paging::translate(virt + i * PAGE_SIZE);
        if (phys) {
            arch::paging::unmap_page(virt + i * PAGE_SIZE);
            deallocate_frame(phys);
        }
    }
}

MemoryStats get_stats() {
    MemoryStats stats;
    stats.total_frames = memory::free_frame_count() + memory::used_frame_count();
    stats.free_frames = memory::free_frame_count();
    stats.used_frames = memory::used_frame_count();
    stats.heap_used = g_heap_used;
    stats.heap_free = (g_heap_end - g_heap_start) - g_heap_used;
    return stats;
}

} // namespace memory