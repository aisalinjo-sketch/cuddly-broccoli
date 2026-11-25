// Global Descriptor Table (GDT) for x86_64

#include "arch.hpp"
#include "common.hpp"

namespace arch {
namespace gdt {

// GDT entry structure
struct alignas(8) Entry {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  limit_high_flags;
    u8  base_high;

    constexpr Entry() : limit_low(0), base_low(0), base_mid(0), access(0), limit_high_flags(0), base_high(0) {}

    static Entry code_segment(u8 dpl = 0) {
        Entry e;
        e.limit_low = 0xFFFF;
        e.base_low = 0;
        e.base_mid = 0;
        // Present=1, DPL=dpl, Type=Code/Execute/Read (0xA), S=1 (code/data)
        e.access = 0x80 | (dpl << 5) | 0x1A;
        // Granularity=1 (4KB), 64-bit=1, Limit high=0xF
        e.limit_high_flags = 0xA0 | 0xF;
        e.base_high = 0;
        return e;
    }

    static Entry data_segment(u8 dpl = 0) {
        Entry e;
        e.limit_low = 0xFFFF;
        e.base_low = 0;
        e.base_mid = 0;
        // Present=1, DPL=dpl, Type=Data/Read/Write (0x2), S=1
        e.access = 0x80 | (dpl << 5) | 0x12;
        // Granularity=1 (4KB), 64-bit=0, Limit high=0xF
        e.limit_high_flags = 0x80 | 0xF;
        e.base_high = 0;
        return e;
    }

    static Entry tss_segment(paddr_t base, u32 limit) {
        Entry e;
        e.limit_low = limit & 0xFFFF;
        e.base_low = base & 0xFFFF;
        e.base_mid = (base >> 16) & 0xFF;
        // Present=1, DPL=0, Type=Available TSS (0x9), S=0 (system)
        e.access = 0x89;
        // Granularity=0 (bytes), Limit high = (limit >> 16) & 0xF
        e.limit_high_flags = (limit >> 16) & 0xF;
        e.base_high = (base >> 24) & 0xFF;
        return e;
    }

    static Entry tss_segment_high(u64 base_high) {
        Entry e;
        e.limit_low = base_high & 0xFFFF;
        e.base_low = (base_high >> 16) & 0xFFFF;
        e.base_mid = 0;
        e.access = 0;
        e.limit_high_flags = 0;
        e.base_high = 0;
        return e;
    }
};

// GDT descriptor for LGDT instruction
struct alignas(8) Descriptor {
    u16 limit;
    u64 base;
};

// TSS structure (Task State Segment)
struct alignas(16) TSS {
    u32 reserved0;
    u64 rsp[3];           // Stack pointers for privilege levels 0, 1, 2
    u64 reserved1;
    u64 ist[7];           // Interrupt Stack Table
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;

    constexpr TSS() : reserved0(0), rsp{0, 0, 0}, reserved1(0), ist{0}, reserved2(0), reserved3(0), iomap_base(sizeof(TSS)) {}
};

// Maximum GDT entries
constexpr usize MAX_ENTRIES = 7;

// GDT entries
alignas(4096) static Entry gdt_entries[MAX_ENTRIES];
static Descriptor gdt_descriptor;
static TSS kernel_tss;

// Segment selectors (index * 8)
constexpr u16 SELECTOR_KERNEL_CODE = 0x08;  // Index 1
constexpr u16 SELECTOR_KERNEL_DATA = 0x10;  // Index 2
constexpr u16 SELECTOR_USER_DATA   = 0x18;  // Index 3
constexpr u16 SELECTOR_USER_CODE   = 0x20;  // Index 4
constexpr u16 SELECTOR_TSS         = 0x28;  // Index 5

extern "C" void gdt_flush(u64 descriptor_ptr);
extern "C" void tss_flush(u16 selector);

void init() {
    // Null descriptor (index 0)
    gdt_entries[0] = Entry();

    // Kernel code segment (index 1)
    gdt_entries[1] = Entry::code_segment(0);

    // Kernel data segment (index 2)
    gdt_entries[2] = Entry::data_segment(0);

    // User data segment (index 3) - DPL 3
    gdt_entries[3] = Entry::data_segment(3);

    // User code segment (index 4) - DPL 3
    gdt_entries[4] = Entry::code_segment(3);

    // TSS descriptor (index 5-6, takes 2 entries for 64-bit)
    paddr_t tss_addr = reinterpret_cast<paddr_t>(&kernel_tss);
    gdt_entries[5] = Entry::tss_segment(tss_addr, sizeof(TSS) - 1);
    gdt_entries[6] = Entry::tss_segment_high(tss_addr >> 32);

    // Set up GDT descriptor
    gdt_descriptor.limit = sizeof(gdt_entries) - 1;
    gdt_descriptor.base = reinterpret_cast<u64>(gdt_entries);

    // Load GDT
    gdt_flush(reinterpret_cast<u64>(&gdt_descriptor));

    // Initialize TSS
    kernel_tss.rsp[0] = 0;  // Will be set per-CPU
    kernel_tss.ist[0] = 0;  // Will be set per-CPU

    // Load TSS
    tss_flush(SELECTOR_TSS);
}

void set_kernel_stack(u64 rsp0) {
    kernel_tss.rsp[0] = rsp0;
}

void set_ist(u8 index, u64 value) {
    if (index < 7) {
        kernel_tss.ist[index] = value;
    }
}

u16 kernel_code_selector() { return SELECTOR_KERNEL_CODE; }
u16 kernel_data_selector() { return SELECTOR_KERNEL_DATA; }
u16 user_code_selector() { return SELECTOR_USER_CODE; }
u16 user_data_selector() { return SELECTOR_USER_DATA; }
u16 tss_selector() { return SELECTOR_TSS; }

} // namespace gdt
} // namespace arch