#pragma once

// Task Management Interface

#include "common.hpp"
#include "arch.hpp"

namespace task {

// Task ID type
using TaskId = u64;

// Task state
enum class TaskState : u8 {
    Ready,
    Running,
    Blocked,
    Sleeping,
    Zombie,
};

// Task priority
enum class Priority : u8 {
    Idle = 0,
    Low = 10,
    Normal = 50,
    High = 100,
    Realtime = 200,
};

// Forward declaration
struct Task;

// Context switch function
extern "C" void context_switch(arch::CpuContext* old_ctx, arch::CpuContext* new_ctx);

// Initialize task subsystem
void init();

// Spawn a kernel thread
TaskId spawn_kernel(const char* name, void (*entry)(), Priority priority = Priority::Normal);

// Spawn a user process
TaskId spawn_user(const char* name, u64 entry_point, u64 user_stack, Priority priority = Priority::Normal);

// Get current task
Task* current_task();

// Set current task
void set_current_task(Task* task);

// Get task by ID
Task* get_task(TaskId id);

// Exit current task
[[noreturn]] void exit(i32 code);

// Yield CPU
void yield();

// Sleep current task
void sleep(u64 ticks);

// Timer tick handler
void timer_tick();

// Test kernel thread
void test_thread();

// Schedule (main scheduler loop)
[[noreturn]] void schedule();

// Spawn init process
void spawn_init();

} // namespace task