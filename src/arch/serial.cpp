// Serial Port Driver (16550 UART)

#include "arch.hpp"
#include "common.hpp"

namespace arch {
namespace serial {

// COM port base addresses
constexpr u16 COM1 = 0x3F8;
constexpr u16 COM2 = 0x2F8;
constexpr u16 COM3 = 0x3E8;
constexpr u16 COM4 = 0x2E8;

// Register offsets
constexpr u16 REG_DATA     = 0;  // Data register (R/W)
constexpr u16 REG_IER      = 1;  // Interrupt Enable Register
constexpr u16 REG_IIR      = 2;  // Interrupt Identification Register (R)
constexpr u16 REG_FCR      = 2;  // FIFO Control Register (W)
constexpr u16 REG_LCR      = 3;  // Line Control Register
constexpr u16 REG_MCR      = 4;  // Modem Control Register
constexpr u16 REG_LSR      = 5;  // Line Status Register
constexpr u16 REG_MSR      = 6;  // Modem Status Register
constexpr u16 REG_SCR      = 7;  // Scratch Register

// LCR bits
constexpr u8 LCR_DLAB = 0x80;  // Divisor Latch Access Bit

// LSR bits
constexpr u8 LSR_DR   = 0x01;  // Data Ready
constexpr u8 LSR_THRE = 0x20;  // Transmitter Holding Register Empty
constexpr u8 LSR_TEMT = 0x40;  // Transmitter Empty

static u16 g_com_port = COM1;
static bool g_initialized = false;

// Port I/O
inline u8 inb(u16 port) {
    u8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outb(u16 port, u8 value) {
    asm volatile("outb %0, %1" :: "a"(value), "Nd"(port));
}

bool is_transmit_empty() {
    return inb(g_com_port + REG_LSR) & LSR_THRE;
}

void init() {
    if (g_initialized) return;

    // Disable interrupts
    outb(g_com_port + REG_IER, 0x00);

    // Set DLAB to access baud rate divisor
    outb(g_com_port + REG_LCR, LCR_DLAB);

    // Set baud rate to 115200 (divisor = 115200/115200 = 1)
    outb(g_com_port + REG_DATA, 0x01);  // LSB
    outb(g_com_port + REG_IER, 0x00);   // MSB

    // 8 bits, no parity, 1 stop bit
    outb(g_com_port + REG_LCR, 0x03);

    // Enable FIFO, clear TX/RX queues, 14-byte threshold
    outb(g_com_port + REG_FCR, 0xC7);

    // Enable interrupts (not used yet), RTS/DTR set
    outb(g_com_port + REG_MCR, 0x0B);

    // Enable interrupts in IER
    outb(g_com_port + REG_IER, 0x01);  // Enable received data interrupt

    g_initialized = true;
}

void write_char(char c) {
    if (!g_initialized) init();

    // Wait for transmitter to be ready
    while (!is_transmit_empty()) {
        arch::cpu::pause();
    }

    // Send character
    outb(g_com_port + REG_DATA, static_cast<u8>(c));

    // If newline, also send carriage return
    if (c == '\n') {
        while (!is_transmit_empty()) {
            arch::cpu::pause();
        }
        outb(g_com_port + REG_DATA, '\r');
    }
}

void write_str(const char* str) {
    if (!str) return;
    while (*str) {
        write_char(*str++);
    }
}

void write_hex(u64 value) {
    char buf[18];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 15; i >= 0; i--) {
        u8 nibble = (value >> (i * 4)) & 0xF;
        buf[17 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    buf[18] = 0;
    write_str(buf);
}

void write_dec(u64 value) {
    char buf[32];
    int len = 0;
    if (value == 0) {
        buf[len++] = '0';
    } else {
        while (value > 0) {
            buf[len++] = '0' + (value % 10);
            value /= 10;
        }
    }
    // Reverse
    for (int i = 0; i < len / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[len - 1 - i];
        buf[len - 1 - i] = tmp;
    }
    buf[len] = 0;
    write_str(buf);
}

} // namespace serial
} // namespace arch