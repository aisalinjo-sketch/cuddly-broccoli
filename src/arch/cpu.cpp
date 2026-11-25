// CPU Control Functions

#include "arch.hpp"
#include "common.hpp"

namespace arch {
namespace cpu {

void enable_interrupts() {
    asm volatile("sti" ::: "memory");
}

void disable_interrupts() {
    asm volatile("cli" ::: "memory");
}

void halt() {
    asm volatile("hlt");
}

void reboot() {
    // Try ACPI reboot first
    // Write to port 0xCF9 (Reset Control Register)
    asm volatile("outb %0, %1" :: "a"((u8)0x06), "Nd"((u16)0xCF9));

    // Fallback: Triple fault by loading invalid IDT
    struct {
        u16 limit;
        u64 base;
    } __attribute__((packed)) idt_ptr = {0, 0};

    asm volatile("lidt [%0]" :: "m"(idt_ptr));
    asm volatile("int3");

    // Should never reach here
    halt();
}

u64 read_cr3() {
    u64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void write_cr3(u64 value) {
    asm volatile("mov %0, %%cr3" :: "r"(value) : "memory");
}

} // namespace cpu
} // namespace arch