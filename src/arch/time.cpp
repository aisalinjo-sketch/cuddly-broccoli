// Time Management (PIT, RTC, TSC)

#include "arch.hpp"
#include "common.hpp"
#include "pic.hpp"

namespace arch {
namespace time {

// PIT (Programmable Interval Timer) registers
constexpr u16 PIT_CHANNEL0 = 0x40;
constexpr u16 PIT_CHANNEL1 = 0x41;
constexpr u16 PIT_CHANNEL2 = 0x42;
constexpr u16 PIT_COMMAND  = 0x43;

// PIT frequency
constexpr u32 PIT_BASE_FREQ = 1193182;  // Hz

// RTC (Real Time Clock) ports
constexpr u16 RTC_INDEX = 0x70;
constexpr u16 RTC_DATA  = 0x71;

// RTC registers
constexpr u8 RTC_SECONDS     = 0x00;
constexpr u8 RTC_MINUTES     = 0x02;
constexpr u8 RTC_HOURS       = 0x04;
constexpr u8 RTC_DAY_OF_WEEK = 0x06;
constexpr u8 RTC_DAY         = 0x07;
constexpr u8 RTC_MONTH       = 0x08;
constexpr u8 RTC_YEAR        = 0x09;
constexpr u8 RTC_REG_A       = 0x0A;
constexpr u8 RTC_REG_B       = 0x0B;
constexpr u8 RTC_REG_C       = 0x0C;

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

// Global tick counter
static volatile u64 g_ticks = 0;
static u64 g_tick_frequency = 1000;  // Default 1000 Hz (1ms)
static u64 g_tsc_per_tick = 0;
static u64 g_cpu_freq_hz = 0;

void init() {
    // Calibrate TSC frequency using PIT
    calibrate_tsc();

    // Initialize PIT for 1ms ticks (1000 Hz)
    u16 divisor = PIT_BASE_FREQ / g_tick_frequency;
    outb(PIT_COMMAND, 0x36);  // Channel 0, lobyte/hibyte, mode 3 (square wave)
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    // Initialize RTC
    init_rtc();
}

void calibrate_tsc() {
    // Use PIT to measure TSC frequency
    // Set PIT channel 0 to one-shot mode, max count
    outb(PIT_COMMAND, 0x30);  // Channel 0, lobyte/hibyte, mode 0
    outb(PIT_CHANNEL0, 0xFF);
    outb(PIT_CHANNEL0, 0xFF);

    // Read TSC start
    u64 tsc_start = rdtsc();

    // Wait for PIT to count down (approximately 54.9ms at max count)
    // Poll PIT channel 0 output bit (bit 6 of port 0x61)
    while (inb(0x61) & 0x20) {
        // Wait for count to reach 0
    }

    u64 tsc_end = rdtsc();
    u64 tsc_elapsed = tsc_end - tsc_start;

    // PIT counts at 1.193182 MHz, max count 65535 = ~54.9ms
    // TSC frequency = tsc_elapsed / (65535 / 1193182)
    g_cpu_freq_hz = (tsc_elapsed * 1193182ULL) / 65535ULL;
    g_tsc_per_tick = g_cpu_freq_hz / g_tick_frequency;
}

void init_rtc() {
    // Disable RTC interrupts
    outb(RTC_INDEX, RTC_REG_B);
    u8 reg_b = inb(RTC_DATA);
    reg_b &= ~0x40;  // Clear SET bit
    outb(RTC_INDEX, RTC_REG_B);
    outb(RTC_DATA, reg_b);

    // Enable update-ended interrupt (optional)
    // outb(RTC_INDEX, RTC_REG_B);
    // outb(RTC_DATA, reg_b | 0x10);
}

// PIT timer interrupt handler (called from IDT)
extern "C" void pit_handler() {
    g_ticks++;

    // Notify scheduler
    extern void task_timer_tick();
    task_timer_tick();

    // Send EOI to PIC
    pic::send_eoi(0);
}

u64 ticks() {
    return g_ticks;
}

u64 uptime_ms() {
    return g_ticks * (1000 / g_tick_frequency);
}

u64 uptime_us() {
    return g_ticks * (1000000 / g_tick_frequency);
}

u64 rdtsc() {
    u32 lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<u64>(hi) << 32) | lo;
}

u64 cpu_frequency_hz() {
    return g_cpu_freq_hz;
}

u64 tsc_per_tick() {
    return g_tsc_per_tick;
}

// Sleep for microseconds using TSC
void sleep_us(u64 us) {
    u64 start = rdtsc();
    u64 target = start + (g_cpu_freq_hz / 1000000) * us;
    while (rdtsc() < target) {
        asm volatile("pause");
    }
}

// Sleep for milliseconds
void sleep_ms(u64 ms) {
    u64 target = g_ticks + ms;
    while (g_ticks < target) {
        asm volatile("hlt");
    }
}

// Get current date/time from RTC
struct DateTime {
    u8 second;
    u8 minute;
    u8 hour;
    u8 day;
    u8 month;
    u16 year;
    u8 day_of_week;
};

DateTime get_datetime() {
    DateTime dt;

    // Wait for RTC update to not be in progress
    while (true) {
        outb(RTC_INDEX, RTC_REG_A);
        if (!(inb(RTC_DATA) & 0x80)) break;
    }

    outb(RTC_INDEX, RTC_SECONDS);
    dt.second = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_MINUTES);
    dt.minute = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_HOURS);
    dt.hour = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_DAY_OF_WEEK);
    dt.day_of_week = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_DAY);
    dt.day = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_MONTH);
    dt.month = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_YEAR);
    dt.year = 2000 + inb(RTC_DATA);  // Assume 2000+

    // Convert from BCD if needed
    auto from_bcd = [](u8 bcd) -> u8 {
        return (bcd >> 4) * 10 + (bcd & 0xF);
    };

    dt.second = from_bcd(dt.second);
    dt.minute = from_bcd(dt.minute);
    dt.hour = from_bcd(dt.hour);
    dt.day = from_bcd(dt.day);
    dt.month = from_bcd(dt.month);
    dt.year = 2000 + from_bcd(static_cast<u8>(dt.year - 2000));
    dt.day_of_week = from_bcd(dt.day_of_week);

    return dt;
}

} // namespace time
} // namespace arch