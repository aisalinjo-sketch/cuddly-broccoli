// Interrupt Descriptor Table (IDT) for x86_64

#include "arch.hpp"
#include "common.hpp"

namespace arch {
namespace idt {

// IDT entry structure (16 bytes for x86_64)
struct alignas(16) Entry {
    u16 offset_low;
    u16 selector;
    u8  ist;
    u8  type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;

    constexpr Entry() : offset_low(0), selector(0), ist(0), type_attr(0), offset_mid(0), offset_high(0), reserved(0) {}

    static Entry interrupt_gate(u64 handler, u16 selector = 0x08, u8 ist = 0) {
        Entry e;
        e.offset_low = handler & 0xFFFF;
        e.selector = selector;
        e.ist = ist;
        // Present=1, DPL=0, Type=Interrupt Gate (0xE)
        e.type_attr = 0x8E;
        e.offset_mid = (handler >> 16) & 0xFFFF;
        e.offset_high = (handler >> 32) & 0xFFFFFFFF;
        e.reserved = 0;
        return e;
    }

    static Entry trap_gate(u64 handler, u16 selector = 0x08, u8 ist = 0) {
        Entry e;
        e.offset_low = handler & 0xFFFF;
        e.selector = selector;
        e.ist = ist;
        // Present=1, DPL=0, Type=Trap Gate (0xF)
        e.type_attr = 0x8F;
        e.offset_mid = (handler >> 16) & 0xFFFF;
        e.offset_high = (handler >> 32) & 0xFFFFFFFF;
        e.reserved = 0;
        return e;
    }

    static Entry user_interrupt_gate(u64 handler, u16 selector = 0x08, u8 ist = 0) {
        Entry e;
        e.offset_low = handler & 0xFFFF;
        e.selector = selector;
        e.ist = ist;
        // Present=1, DPL=3, Type=Interrupt Gate (0xE)
        e.type_attr = 0xEE;
        e.offset_mid = (handler >> 16) & 0xFFFF;
        e.offset_high = (handler >> 32) & 0xFFFFFFFF;
        e.reserved = 0;
        return e;
    }
};

// IDT descriptor for LIDT instruction
struct alignas(8) Descriptor {
    u16 limit;
    u64 base;
};

// Maximum IDT entries (256 vectors)
constexpr usize MAX_ENTRIES = 256;

alignas(4096) static Entry idt_entries[MAX_ENTRIES];
static Descriptor idt_descriptor;

// Exception handler function type
using ExceptionHandler = void (*)(struct InterruptFrame*);

// Interrupt frame pushed by CPU (for exceptions/interrupts)
struct alignas(16) InterruptFrame {
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
    // Error code pushed by some exceptions
    u64 error_code;
};

// Array of exception handlers
static ExceptionHandler exception_handlers[32] = {};
static void (*irq_handlers[16])(InterruptFrame*) = {};

// Assembly interrupt stubs (defined in idt.asm)
extern "C" void* isr_stubs[32];
extern "C" void* irq_stubs[16];

void init() {
    // Clear all entries
    for (usize i = 0; i < MAX_ENTRIES; i++) {
        idt_entries[i] = Entry();
    }

    // Set up exception handlers (0-31) with ISR stubs
    for (usize i = 0; i < 32; i++) {
        idt_entries[i] = Entry::interrupt_gate(reinterpret_cast<u64>(isr_stubs[i]));
    }

    // Set up IRQ handlers (32-47) with IRQ stubs
    for (usize i = 0; i < 16; i++) {
        idt_entries[32 + i] = Entry::interrupt_gate(reinterpret_cast<u64>(irq_stubs[i]));
    }

    // System call entry (vector 0x80) - trap gate for syscall
    idt_entries[0x80] = Entry::trap_gate(reinterpret_cast<u64>(isr_stubs[32]), 0x08, 0);

    // Set up IDT descriptor
    idt_descriptor.limit = sizeof(idt_entries) - 1;
    idt_descriptor.base = reinterpret_cast<u64>(idt_entries);

    // Load IDT
    asm volatile("lidt [%0]" :: "m"(idt_descriptor) : "memory");
}

void set_handler(u8 vector, void (*handler)(InterruptFrame*)) {
    if (vector < 32) {
        exception_handlers[vector] = handler;
    } else if (vector >= 32 && vector < 48) {
        irq_handlers[vector - 32] = handler;
    }
}

// Default exception handler
extern "C" void default_exception_handler(InterruptFrame* frame, u8 vector) {
    const char* exception_names[32] = {
        "Division by zero",
        "Debug",
        "Non-maskable interrupt",
        "Breakpoint",
        "Overflow",
        "Bound range exceeded",
        "Invalid opcode",
        "Device not available",
        "Double fault",
        "Coprocessor segment overrun",
        "Invalid TSS",
        "Segment not present",
        "Stack segment fault",
        "General protection fault",
        "Page fault",
        "Reserved",
        "x87 FPU floating-point error",
        "Alignment check",
        "Machine check",
        "SIMD floating-point exception",
        "Virtualization exception",
        "Control protection exception",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved"
    };

    const char* name = (vector < 32) ? exception_names[vector] : "Unknown exception";

    // Print panic info via serial
    arch::serial::write_str("\n*** EXCEPTION: ");
    arch::serial::write_str(name);
    arch::serial::write_str(" ***\n");
    arch::serial::write_str("Vector: ");

    char buf[32];
    int len = 0;
    u8 v = vector;
    if (v >= 100) { buf[len++] = '0' + v / 100; v %= 100; }
    if (v >= 10) { buf[len++] = '0' + v / 10; v %= 10; }
    buf[len++] = '0' + v;
    buf[len] = 0;
    arch::serial::write_str(buf);
    arch::serial::write_str("\n");

    arch::serial::write_str("RIP: 0x");
    // Print RIP
    u64 rip = frame->rip;
    for (int i = 15; i >= 0; i--) {
        u8 nibble = (rip >> (i * 4)) & 0xF;
        buf[15 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    buf[16] = 0;
    arch::serial::write_str(buf);
    arch::serial::write_str("\n");

    arch::serial::write_str("RFLAGS: 0x");
    u64 rflags = frame->rflags;
    for (int i = 15; i >= 0; i--) {
        u8 nibble = (rflags >> (i * 4)) & 0xF;
        buf[15 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    buf[16] = 0;
    arch::serial::write_str(buf);
    arch::serial::write_str("\n");

    arch::serial::write_str("RSP: 0x");
    u64 rsp = frame->rsp;
    for (int i = 15; i >= 0; i--) {
        u8 nibble = (rsp >> (i * 4)) & 0xF;
        buf[15 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    buf[16] = 0;
    arch::serial::write_str(buf);
    arch::serial::write_str("\n");

    if (vector == 14) {  // Page fault - print CR2
        u64 cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        arch::serial::write_str("CR2 (fault addr): 0x");
        for (int i = 15; i >= 0; i--) {
            u8 nibble = (cr2 >> (i * 4)) & 0xF;
            buf[15 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }
        buf[16] = 0;
        arch::serial::write_str(buf);
        arch::serial::write_str("\n");
        arch::serial::write_str("Error code: ");
        len = 0;
        u64 err = frame->error_code;
        if (err == 0) {
            buf[len++] = '0';
        } else {
            while (err > 0) {
                buf[len++] = '0' + (err % 10);
                err /= 10;
            }
        }
        // Reverse
        for (int i = 0; i < len / 2; i++) {
            char tmp = buf[i];
            buf[i] = buf[len - 1 - i];
            buf[len - 1 - i] = tmp;
        }
        buf[len] = 0;
        arch::serial::write_str(buf);
        arch::serial::write_str("\n");
    }

    // Halt
    arch::cpu::halt();
}

// Default IRQ handler
extern "C" void default_irq_handler(InterruptFrame* frame, u8 irq) {
    // Just acknowledge for now
    if (irq < 16) {
        // Send EOI to PIC
        if (irq >= 8) {
            // Slave PIC
            asm volatile("outb %0, %1" :: "a"((u8)0x20), "Nd"(0xA0));
        }
        // Master PIC
        asm volatile("outb %0, %1" :: "a"((u8)0x20), "Nd"(0x20));
    }
}

// System call handler
extern "C" void syscall_handler(InterruptFrame* frame) {
    // System call handling will be implemented in task/syscall.cpp
    // For now, just return
    frame->rip += 2; // Skip syscall instruction
}

} // namespace idt
} // namespace arch