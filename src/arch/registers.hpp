#pragma once

// CPU Register structures for context switching

#include "common.hpp"

namespace arch {

// General purpose registers (matches x86_64 System V ABI)
struct alignas(16) GeneralRegisters {
    u64 rax, rbx, rcx, rdx;
    u64 rsi, rdi, rbp, rsp;
    u64 r8,  r9,  r10, r11;
    u64 r12, r13, r14, r15;
    u64 rip, rflags;
    u64 cs, ss, ds, es, fs, gs;
};

// FPU/SSE/AVX state (512 bytes for FXSAVE, 2560 for XSAVE)
struct alignas(64) FpuState {
    u16 fcw, fsw, ftw, fop;
    u64 fpu_ip, fpu_dp;
    u32 mxcsr, mxcsr_mask;
    u64 reserved1;
    u8 st_space[128];   // 8 x 80-bit registers
    u8 xmm_space[256];  // 16 x 128-bit registers
    u8 ymm_space[512];  // 16 x 256-bit registers (AVX)
    u8 zmm_space[1024]; // 32 x 512-bit registers (AVX-512)
    u8 padding[96];
};

// Complete CPU context for task switching
struct alignas(16) CpuContext {
    GeneralRegisters general;
    FpuState fpu;

    CpuContext() {
        // Zero initialize
        for (usize i = 0; i < sizeof(*this); i++) {
            reinterpret_cast<u8*>(this)[i] = 0;
        }
    }

    // Create kernel thread context
    static CpuContext new_kernel(u64 entry_point, u64 kernel_stack_top) {
        CpuContext ctx;
        ctx.general.rip = entry_point;
        ctx.general.rsp = kernel_stack_top;
        ctx.general.rflags = 0x202;  // IF=1 (interrupts enabled)
        ctx.general.cs = 0x08;       // Kernel code segment
        ctx.general.ss = 0x10;       // Kernel data segment
        ctx.general.ds = 0x10;
        ctx.general.es = 0x10;
        ctx.general.fs = 0x10;
        ctx.general.gs = 0x10;
        return ctx;
    }

    // Create user process context
    static CpuContext new_user(u64 entry_point, u64 user_stack_top, u64 kernel_stack_top) {
        CpuContext ctx;
        ctx.general.rip = entry_point;
        ctx.general.rsp = user_stack_top;
        ctx.general.rflags = 0x202;  // IF=1
        ctx.general.cs = 0x23;       // User code segment (index 4, RPL=3)
        ctx.general.ss = 0x2B;       // User data segment (index 5, RPL=3)
        ctx.general.ds = 0x2B;
        ctx.general.es = 0x2B;
        ctx.general.fs = 0x2B;
        ctx.general.gs = 0x2B;
        // Store kernel stack for syscall entry
        // This would be in a separate field or TSS
        return ctx;
    }
};

// Read/write CR3
inline u64 read_cr3() {
    u64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

inline void write_cr3(u64 value) {
    asm volatile("mov %0, %%cr3" :: "r"(value) : "memory");
}

// Read CR2 (page fault address)
inline u64 read_cr2() {
    u64 cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

// Read/write CR0
inline u64 read_cr0() {
    u64 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

inline void write_cr0(u64 value) {
    asm volatile("mov %0, %%cr0" :: "r"(value) : "memory");
}

// Read/write CR4
inline u64 read_cr4() {
    u64 cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    return cr4;
}

inline void write_cr4(u64 value) {
    asm volatile("mov %0, %%cr4" :: "r"(value) : "memory");
}

// Enable/disable interrupts
inline void enable_interrupts() {
    asm volatile("sti" ::: "memory");
}

inline void disable_interrupts() {
    asm volatile("cli" ::: "memory");
}

inline bool interrupts_enabled() {
    u64 rflags;
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    return rflags & (1ULL << 9);
}

// Halt CPU
inline void halt() {
    asm volatile("hlt");
}

// Pause (for spinlocks)
inline void pause() {
    asm volatile("pause");
}

// Memory barrier
inline void memory_barrier() {
    asm volatile("mfence" ::: "memory");
}

// Read TSC
inline u64 rdtsc() {
    u32 lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<u64>(hi) << 32) | lo;
}

// CR8 (TPR - Task Priority Register) for x2APIC
inline u64 read_cr8() {
    u64 cr8;
    asm volatile("mov %%cr8, %0" : "=r"(cr8));
    return cr8;
}

inline void write_cr8(u64 value) {
    asm volatile("mov %0, %%cr8" :: "r"(value) : "memory");
}

} // namespace arch