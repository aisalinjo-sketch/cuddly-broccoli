// Novaria OS Kernel - Main Entry Point

#include "common.hpp"
#include "arch.hpp"
#include "memory.hpp"
#include "task.hpp"
#include "drivers.hpp"
#include "ui.hpp"
#include "log.hpp"
#include "panic.hpp"

extern "C" {

// Kernel entry point called from assembly bootstrap
void kernel_main() {
    // Initialize architecture subsystems
    arch::init();

    // Initialize UI (serial console)
    ui::init();

    LOG_INFO("Novaria OS Kernel v0.1.0");
    LOG_INFO("Booting on x86_64");

    // Initialize memory management
    // Physical memory offset from linker script / bootloader
    paddr_t phys_offset = 0xFFFF800000000000ULL;
    paddr_t mem_size = 0x80000000;  // 2GB assumed
    memory::init(phys_offset, mem_size);
    LOG_INFO("Memory management initialized");

    // Initialize device drivers
    drivers::init();
    LOG_INFO("Device drivers initialized");

    // Initialize task scheduler
    task::init();
    LOG_INFO("Task scheduler initialized");

    // Spawn init process
    task::spawn_init();
    LOG_INFO("Init process spawned");

    // Spawn test thread
    task::spawn_kernel("test", task::test_thread, task::Priority::Normal);
    LOG_INFO("Test thread spawned");

    // Enter main scheduler loop
    LOG_INFO("Entering scheduler...");
    task::schedule();
}

} // extern "C"

// Out-of-memory handler
extern "C" void __cxa_pure_virtual() {
    panic("Pure virtual function called");
}

// C++ new/delete operators (use kernel heap)
void* operator new(size_t size) {
    return memory::kmalloc(size);
}

void* operator new[](size_t size) {
    return memory::kmalloc(size);
}

void operator delete(void* ptr) noexcept {
    memory::kfree(ptr);
}

void operator delete[](void* ptr) noexcept {
    memory::kfree(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    memory::kfree(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    memory::kfree(ptr);
}