// Page Table Management for x86_64

#include "arch.hpp"
#include "common.hpp"

namespace arch {
namespace paging {

// Page table flags
constexpr u64 FLAG_PRESENT       = 1ULL << 0;
constexpr u64 FLAG_WRITE         = 1ULL << 1;
constexpr u64 FLAG_USER          = 1ULL << 2;
constexpr u64 FLAG_WRITE_THROUGH = 1ULL << 3;
constexpr u64 FLAG_CACHE_DISABLE = 1ULL << 4;
constexpr u64 FLAG_ACCESSED      = 1ULL << 5;
constexpr u64 FLAG_DIRTY         = 1ULL << 6;
constexpr u64 FLAG_HUGE          = 1ULL << 7;
constexpr u64 FLAG_GLOBAL        = 1ULL << 8;
constexpr u64 FLAG_NO_EXECUTE    = 1ULL << 63;

// Common flag combinations
constexpr u64 KERNEL_PAGE_FLAGS = FLAG_PRESENT | FLAG_WRITE | FLAG_NO_EXECUTE;
constexpr u64 USER_PAGE_FLAGS   = FLAG_PRESENT | FLAG_WRITE | FLAG_USER | FLAG_NO_EXECUTE;
constexpr u64 DEVICE_PAGE_FLAGS = FLAG_PRESENT | FLAG_WRITE | FLAG_NO_EXECUTE | FLAG_CACHE_DISABLE;

// Page table entry (64-bit)
struct PageTableEntry {
    u64 value;

    constexpr PageTableEntry() : value(0) {}
    constexpr PageTableEntry(u64 v) : value(v) {}

    bool present() const { return value & FLAG_PRESENT; }
    void set_present(bool p) { value = (value & ~FLAG_PRESENT) | (p ? FLAG_PRESENT : 0); }

    bool writable() const { return value & FLAG_WRITE; }
    void set_writable(bool w) { value = (value & ~FLAG_WRITE) | (w ? FLAG_WRITE : 0); }

    bool user() const { return value & FLAG_USER; }
    void set_user(bool u) { value = (value & ~FLAG_USER) | (u ? FLAG_USER : 0); }

    bool no_execute() const { return value & FLAG_NO_EXECUTE; }
    void set_no_execute(bool n) { value = (value & ~FLAG_NO_EXECUTE) | (n ? FLAG_NO_EXECUTE : 0); }

    paddr_t address() const { return value & 0x000FFFFFFFFFF000ULL; }
    void set_address(paddr_t addr) { value = (value & ~0x000FFFFFFFFFF000ULL) | (addr & 0x000FFFFFFFFFF000ULL); }

    u64 flags() const { return value & ~0x000FFFFFFFFFF000ULL; }
    void set_flags(u64 f) { value = (value & 0x000FFFFFFFFFF000ULL) | (f & ~0x000FFFFFFFFFF000ULL); }
};

// Page table (512 entries = 4KB)
struct alignas(4096) PageTable {
    PageTableEntry entries[512];

    PageTableEntry& operator[](usize index) { return entries[index]; }
    const PageTableEntry& operator[](usize index) const { return entries[index]; }

    void clear() {
        for (usize i = 0; i < 512; i++) {
            entries[i] = PageTableEntry();
        }
    }
};

// Physical memory offset for direct mapping
static paddr_t g_physical_memory_offset = 0;

// Active page table (kernel page table root)
static PageTable* g_active_page_table = nullptr;

// Get physical memory offset
paddr_t physical_memory_offset() {
    return g_physical_memory_offset;
}

// Initialize paging with physical memory offset from bootloader
void init(paddr_t physical_memory_offset) {
    g_physical_memory_offset = physical_memory_offset;

    // Get current CR3 (page table root)
    u64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    paddr_t pml4_phys = cr3 & 0x000FFFFFFFFFF000ULL;

    // Map physical address to virtual using direct mapping
    vaddr_t pml4_virt = pml4_phys + g_physical_memory_offset;
    g_active_page_table = reinterpret_cast<PageTable*>(pml4_virt);
}

PageTable* active_page_table() {
    return g_active_page_table;
}

// Create a new page table for a process
PageTable* create_page_table() {
    // Allocate a physical frame for the new PML4
    paddr_t frame = allocate_frame();
    if (frame == 0) {
        return nullptr;
    }

    // Map to virtual address
    vaddr_t virt = frame + g_physical_memory_offset;
    PageTable* table = reinterpret_cast<PageTable*>(virt);

    // Zero the page table
    table->clear();

    // Recursively map the PML4 into itself at entry 511 (for easy access)
    table->entries[511].set_address(frame);
    table->entries[511].set_flags(FLAG_PRESENT | FLAG_WRITE);

    return table;
}

// Switch to a different page table (for process context switch)
void switch_page_table(PageTable* page_table) {
    paddr_t phys = reinterpret_cast<paddr_t>(page_table) - g_physical_memory_offset;
    asm volatile("mov %0, %%cr3" :: "r"(phys) : "memory");
    g_active_page_table = page_table;
}

// Map a page to a frame with given flags
bool map_page(vaddr_t virt, paddr_t phys, u64 flags) {
    if (!g_active_page_table) return false;

    // Ensure alignment
    virt = PAGE_ALIGN_DOWN(virt);
    phys = PAGE_ALIGN_DOWN(phys);

    // Walk page tables
    PageTable* pml4 = g_active_page_table;

    // PML4 index (bits 39-47)
    usize pml4_idx = (virt >> 39) & 0x1FF;
    if (!pml4->entries[pml4_idx].present()) {
        paddr_t frame = allocate_frame();
        if (frame == 0) return false;
        pml4->entries[pml4_idx].set_address(frame);
        pml4->entries[pml4_idx].set_flags(FLAG_PRESENT | FLAG_WRITE | FLAG_USER);
        // Zero the new page directory
        PageTable* pdpt = reinterpret_cast<PageTable*>(frame + g_physical_memory_offset);
        pdpt->clear();
    }

    PageTable* pdpt = reinterpret_cast<PageTable*>(pml4->entries[pml4_idx].address() + g_physical_memory_offset);

    // PDPT index (bits 30-38)
    usize pdpt_idx = (virt >> 30) & 0x1FF;
    if (!pdpt->entries[pdpt_idx].present()) {
        paddr_t frame = allocate_frame();
        if (frame == 0) return false;
        pdpt->entries[pdpt_idx].set_address(frame);
        pdpt->entries[pdpt_idx].set_flags(FLAG_PRESENT | FLAG_WRITE | FLAG_USER);
        // Zero the new page directory
        PageTable* pd = reinterpret_cast<PageTable*>(frame + g_physical_memory_offset);
        pd->clear();
    }

    PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx].address() + g_physical_memory_offset);

    // PD index (bits 21-29)
    usize pd_idx = (virt >> 21) & 0x1FF;
    if (!pd->entries[pd_idx].present()) {
        paddr_t frame = allocate_frame();
        if (frame == 0) return false;
        pd->entries[pd_idx].set_address(frame);
        pd->entries[pd_idx].set_flags(FLAG_PRESENT | FLAG_WRITE | FLAG_USER);
        // Zero the new page table
        PageTable* pt = reinterpret_cast<PageTable*>(frame + g_physical_memory_offset);
        pt->clear();
    }

    PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx].address() + g_physical_memory_offset);

    // PT index (bits 12-20)
    usize pt_idx = (virt >> 12) & 0x1FF;
    pt->entries[pt_idx].set_address(phys);
    pt->entries[pt_idx].set_flags(flags | FLAG_PRESENT);

    // Invalidate TLB for this page
    asm volatile("invlpg [%0]" :: "r"(virt) : "memory");

    return true;
}

// Unmap a page
bool unmap_page(vaddr_t virt) {
    if (!g_active_page_table) return false;

    virt = PAGE_ALIGN_DOWN(virt);

    PageTable* pml4 = g_active_page_table;
    usize pml4_idx = (virt >> 39) & 0x1FF;
    if (!pml4->entries[pml4_idx].present()) return false;

    PageTable* pdpt = reinterpret_cast<PageTable*>(pml4->entries[pml4_idx].address() + g_physical_memory_offset);
    usize pdpt_idx = (virt >> 30) & 0x1FF;
    if (!pdpt->entries[pdpt_idx].present()) return false;

    PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx].address() + g_physical_memory_offset);
    usize pd_idx = (virt >> 21) & 0x1FF;
    if (!pd->entries[pd_idx].present()) return false;

    PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx].address() + g_physical_memory_offset);
    usize pt_idx = (virt >> 12) & 0x1FF;

    pt->entries[pt_idx] = PageTableEntry();

    // Invalidate TLB
    asm volatile("invlpg [%0]" :: "r"(virt) : "memory");

    return true;
}

// Translate virtual to physical address
paddr_t translate(vaddr_t virt) {
    if (!g_active_page_table) return 0;

    PageTable* pml4 = g_active_page_table;
    usize pml4_idx = (virt >> 39) & 0x1FF;
    if (!pml4->entries[pml4_idx].present()) return 0;

    PageTable* pdpt = reinterpret_cast<PageTable*>(pml4->entries[pml4_idx].address() + g_physical_memory_offset);
    usize pdpt_idx = (virt >> 30) & 0x1FF;
    if (!pdpt->entries[pdpt_idx].present()) return 0;

    PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx].address() + g_physical_memory_offset);
    usize pd_idx = (virt >> 21) & 0x1FF;
    if (!pd->entries[pd_idx].present()) return 0;

    PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx].address() + g_physical_memory_offset);
    usize pt_idx = (virt >> 12) & 0x1FF;
    if (!pt->entries[pt_idx].present()) return 0;

    return pt->entries[pt_idx].address() | (virt & 0xFFF);
}

// Map a range of pages
bool map_range(vaddr_t virt_start, paddr_t phys_start, usize page_count, u64 flags) {
    for (usize i = 0; i < page_count; i++) {
        if (!map_page(virt_start + i * PAGE_SIZE, phys_start + i * PAGE_SIZE, flags)) {
            // Rollback on failure
            for (usize j = 0; j < i; j++) {
                unmap_page(virt_start + j * PAGE_SIZE);
            }
            return false;
        }
    }
    return true;
}

// Unmap a range of pages
void unmap_range(vaddr_t virt_start, usize page_count) {
    for (usize i = 0; i < page_count; i++) {
        unmap_page(virt_start + i * PAGE_SIZE);
    }
}

// Map kernel page
bool map_kernel_page(vaddr_t virt, paddr_t phys) {
    return map_page(virt, phys, KERNEL_PAGE_FLAGS);
}

// Map user page
bool map_user_page(vaddr_t virt, paddr_t phys) {
    return map_page(virt, phys, USER_PAGE_FLAGS);
}

// Map MMIO region
bool map_mmio(vaddr_t virt, paddr_t phys, usize page_count) {
    return map_range(virt, phys, page_count, DEVICE_PAGE_FLAGS);
}

// Forward declaration for frame allocator
paddr_t allocate_frame();

} // namespace paging
} // namespace arch