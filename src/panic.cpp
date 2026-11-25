// Panic Handler

#include "common.hpp"
#include "arch.hpp"

[[noreturn]] void panic(const char* message) {
    arch::cpu::disable_interrupts();

    arch::serial::write_str("\n*** KERNEL PANIC ***\n");
    arch::serial::write_str(message);
    arch::serial::write_str("\n");

    // Halt CPU
    while (true) {
        arch::cpu::halt();
    }
}

[[noreturn]] void panic(const char* file, int line, const char* message) {
    arch::cpu::disable_interrupts();

    arch::serial::write_str("\n*** KERNEL PANIC ***\n");
    arch::serial::write_str("  at ");
    arch::serial::write_str(file);
    arch::serial::write_str(":");
    arch::serial::write_dec(line);
    arch::serial::write_str("\n");
    arch::serial::write_str("  ");
    arch::serial::write_str(message);
    arch::serial::write_str("\n");

    // Print stack trace (simplified)
    arch::serial::write_str("\nStack trace:\n");
    // Would need frame pointer walking for real stack trace

    while (true) {
        arch::cpu::halt();
    }
}

// Assertion handler
[[noreturn]] void assert_fail(const char* expr, const char* file, int line, const char* func) {
    panic(file, line, expr);
}

extern "C" void __assert_fail(const char* expr, const char* file, int line, const char* func) {
    assert_fail(expr, file, line, func);
}