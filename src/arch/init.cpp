// Architecture Initialization

#include "arch.hpp"
#include "common.hpp"

extern "C" void* multiboot_info_ptr;

namespace arch {

void init() {
    // Initialize GDT
    gdt::init();

    // Initialize IDT
    idt::init();

    // Initialize PIC
    pic::init();

    // Initialize serial port early for logging
    serial::init();

    // Initialize time (PIT, RTC, TSC)
    time::init();

    // Enable PIT interrupt (IRQ 0)
    pic::enable_irq(0);

    // Get physical memory offset from multiboot info
    // For simplicity, we assume the bootloader provides a direct mapping
    // at a known offset (typically 0xFFFF800000000000 for higher-half kernel)
    // In a real implementation, this would be parsed from multiboot info
    paddr_t phys_offset = 0xFFFF800000000000ULL;

    // Initialize paging
    paging::init(phys_offset);

    // Enable interrupts
    cpu::enable_interrupts();
}

void halt() {
    cpu::halt();
}

void reboot() {
    cpu::reboot();
}

paddr_t get_multiboot_info() {
    return reinterpret_cast<paddr_t>(multiboot_info_ptr);
}

} // namespace arch