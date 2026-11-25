#pragma once

// Architecture-specific declarations
// This module provides x86_64 specific functionality

#include "common.hpp"

// Forward declare PageTable
namespace arch {
namespace paging {
    struct PageTable;
}
}

// Forward declarations
namespace arch {
namespace gdt {
    void init();
    void set_kernel_stack(u64 rsp0);
    void set_ist(u8 index, u64 value);
    u16 kernel_code_selector();
    u16 kernel_data_selector();
    u16 user_code_selector();
    u16 user_data_selector();
    u16 tss_selector();
}

namespace idt {
    void init();
    void set_handler(u8 vector, void (*handler)());
}

namespace paging {
    void init(paddr_t physical_memory_offset);
    paddr_t physical_memory_offset();
    PageTable* active_page_table();
    PageTable* create_page_table();
    void switch_page_table(PageTable* page_table);
    bool map_page(vaddr_t virt, paddr_t phys, u64 flags);
    bool unmap_page(vaddr_t virt);
    paddr_t translate(vaddr_t virt);
    bool map_range(vaddr_t virt_start, paddr_t phys_start, usize page_count, u64 flags);
    void unmap_range(vaddr_t virt_start, usize page_count);
    bool map_kernel_page(vaddr_t virt, paddr_t phys);
    bool map_user_page(vaddr_t virt, paddr_t phys);
    bool map_mmio(vaddr_t virt, paddr_t phys, usize page_count);

    // Page flags
    constexpr u64 KERNEL_PAGE_FLAGS = 0x3;  // PRESENT | WRITE | NO_EXECUTE
    constexpr u64 USER_PAGE_FLAGS = 0x7;    // PRESENT | WRITE | USER | NO_EXECUTE
    constexpr u64 DEVICE_PAGE_FLAGS = 0x13; // PRESENT | WRITE | NO_EXECUTE | CACHE_DISABLED
}

namespace serial {
    void init();
    void write_str(const char* str);
    void write_char(char c);
}

namespace time {
    void init();
    u64 ticks();
    u64 uptime_ms();
    u64 rdtsc();
    u64 cpu_frequency_hz();
}

namespace cpu {
    void enable_interrupts();
    void disable_interrupts();
    void halt();
    void reboot();
    u64 read_cr3();
    void write_cr3(u64 value);
}

namespace pic {
    void init();
    void enable_irq(u8 irq);
    void disable_irq(u8 irq);
    void send_eoi(u8 irq);
}

// Initialize all architecture subsystems
void init();

} // namespace arch