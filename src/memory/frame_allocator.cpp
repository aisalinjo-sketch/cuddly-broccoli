// Physical Frame Allocator (Bitmap-based)

#include "memory.hpp"
#include "arch.hpp"
#include "common.hpp"

namespace memory {

// Frame allocator state
static paddr_t g_memory_base = 0;
static paddr_t g_memory_size = 0;
static u64* g_bitmap = nullptr;
static usize g_total_frames = 0;
static usize g_free_frames = 0;
static usize g_bitmap_size = 0;  // in 64-bit words

// Multiboot memory map entry
struct MemoryMapEntry {
    u64 base_addr;
    u64 length;
    u32 type;
    u32 reserved;
};

// Find memory map from multiboot info
static void parse_memory_map(paddr_t multiboot_info) {
    // Simplified: assume identity mapping with 2GB available
    // In a real implementation, parse the multiboot memory map
    g_memory_base = 0x100000;  // Start at 1MB
    g_memory_size = 0x80000000;  // 2GB
    g_total_frames = g_memory_size / PAGE_SIZE;
    g_free_frames = g_total_frames;
    g_bitmap_size = (g_total_frames + 63) / 64;

    // Place bitmap after kernel (at 2MB for now)
    g_bitmap = reinterpret_cast<u64*>(0x200000 + arch::paging::physical_memory_offset());

    // Clear bitmap (all frames free initially)
    for (usize i = 0; i < g_bitmap_size; i++) {
        g_bitmap[i] = 0;
    }

    // Mark kernel frames as used (first ~4MB)
    usize kernel_frames = (4 * 1024 * 1024) / PAGE_SIZE;
    for (usize i = 0; i < kernel_frames; i++) {
        usize word = i / 64;
        usize bit = i % 64;
        g_bitmap[word] |= (1ULL << bit);
        g_free_frames--;
    }

    // Mark bitmap frames as used
    usize bitmap_frames = (g_bitmap_size * 8 + PAGE_SIZE - 1) / PAGE_SIZE;
    usize bitmap_start_frame = (reinterpret_cast<paddr_t>(g_bitmap) - arch::paging::physical_memory_offset() - g_memory_base) / PAGE_SIZE;
    for (usize i = 0; i < bitmap_frames; i++) {
        usize frame = bitmap_start_frame + i;
        if (frame < g_total_frames) {
            usize word = frame / 64;
            usize bit = frame % 64;
            if (!(g_bitmap[word] & (1ULL << bit))) {
                g_bitmap[word] |= (1ULL << bit);
                g_free_frames--;
            }
        }
    }
}

void init(paddr_t physical_memory_offset, paddr_t memory_size) {
    parse_memory_map(arch::get_multiboot_info());

    // Identity map the bitmap region in page tables
    paddr_t bitmap_phys = reinterpret_cast<paddr_t>(g_bitmap) - physical_memory_offset;
    arch::paging::map_range(
        reinterpret_cast<vaddr_t>(g_bitmap),
        bitmap_phys,
        (g_bitmap_size * 8 + 511) / 512,  // Number of pages
        arch::paging::KERNEL_PAGE_FLAGS
    );
}

paddr_t allocate_frame() {
    if (g_free_frames == 0) {
        return 0;
    }

    // Find first free frame
    for (usize word_idx = 0; word_idx < g_bitmap_size; word_idx++) {
        u64 word = g_bitmap[word_idx];
        if (word != ~0ULL) {
            // Find first zero bit
            usize bit = __builtin_ctzll(~word);
            usize frame_idx = word_idx * 64 + bit;

            if (frame_idx >= g_total_frames) {
                return 0;
            }

            // Mark as used
            g_bitmap[word_idx] |= (1ULL << bit);
            g_free_frames--;

            return g_memory_base + frame_idx * PAGE_SIZE;
        }
    }

    return 0;
}

void deallocate_frame(paddr_t frame) {
    if (frame < g_memory_base) return;

    usize frame_idx = (frame - g_memory_base) / PAGE_SIZE;
    if (frame_idx >= g_total_frames) return;

    usize word_idx = frame_idx / 64;
    usize bit = frame_idx % 64;

    if (g_bitmap[word_idx] & (1ULL << bit)) {
        g_bitmap[word_idx] &= ~(1ULL << bit);
        g_free_frames++;
    }
}

usize free_frame_count() {
    return g_free_frames;
}

usize used_frame_count() {
    return g_total_frames - g_free_frames;
}

} // namespace memory