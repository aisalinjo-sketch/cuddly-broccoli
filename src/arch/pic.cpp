// PIC (Programmable Interrupt Controller) Initialization

#include "arch.hpp"
#include "common.hpp"

namespace arch {
namespace pic {

// PIC ports
constexpr u16 PIC1_COMMAND = 0x20;
constexpr u16 PIC1_DATA    = 0x21;
constexpr u16 PIC2_COMMAND = 0xA0;
constexpr u16 PIC2_DATA    = 0xA1;

// ICW1
constexpr u8 ICW1_ICW4 = 0x01;  // ICW4 needed
constexpr u8 ICW1_SINGLE = 0x02; // Single (cascade) mode
constexpr u8 ICW1_INTERVAL4 = 0x04; // Call address interval 4 (8)
constexpr u8 ICW1_LEVEL = 0x08;  // Level triggered (edge)
constexpr u8 ICW1_INIT = 0x10;   // Initialization - required!

// ICW4
constexpr u8 ICW4_8086 = 0x01;   // 8086/88 mode
constexpr u8 ICW4_AUTO = 0x02;   // Auto EOI
constexpr u8 ICW4_BUF_SLAVE = 0x08; // Buffered mode/slave
constexpr u8 ICW4_BUF_MASTER = 0x0C; // Buffered mode/master
constexpr u8 ICW4_SFNM = 0x10;   // Special fully nested mode

// Port I/O
inline u8 inb(u16 port) {
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outb(u16 port, u8 value) {
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

inline void io_wait() {
    asm volatile("outb %%al, $0x80" :: "a"(0));
}

void init() {
    // Save current masks
    u8 mask1 = inb(PIC1_DATA);
    u8 mask2 = inb(PIC2_DATA);

    // Start initialization sequence (cascade mode)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // ICW2: Vector offsets
    outb(PIC1_DATA, 0x20);  // Master PIC vectors 0x20-0x27
    io_wait();
    outb(PIC2_DATA, 0x28);  // Slave PIC vectors 0x28-0x2F
    io_wait();

    // ICW3: Cascade identity
    outb(PIC1_DATA, 0x04);  // Slave at IRQ2
    io_wait();
    outb(PIC2_DATA, 0x02);  // Cascade identity
    io_wait();

    // ICW4: 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

// Enable IRQ
void enable_irq(u8 irq) {
    u16 port;
    u8 value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// Disable IRQ
void disable_irq(u8 irq) {
    u16 port;
    u8 value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

// Send EOI
void send_eoi(u8 irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);
    }
    outb(PIC1_COMMAND, 0x20);
}

} // namespace pic
} // namespace arch