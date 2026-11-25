// Priority-based Round-Robin Scheduler

#include "task.hpp"
#include "memory.hpp"
#include "arch.hpp"
#include "common.hpp"

namespace task {

// Task control block
struct Task {
    TaskId id;
    char name[64];
    TaskState state;
    Priority priority;
    arch::CpuContext context;
    paddr_t kernel_stack;
    u64 user_stack;
    arch::paging::PageTable* page_table;
    TaskId parent;
    i32 exit_code;
    u64 sleep_until;
    Task* next;  // For run queue
    Task* prev;
};

// Scheduler state
static Task* g_tasks[4096] = {nullptr};
static usize g_task_count = 0;
static TaskId g_next_task_id = 1;
static Task* g_current_task = nullptr;
static Task* g_idle_task = nullptr;
static SpinLock g_scheduler_lock;

// Run queues per priority (8 priority levels)
static Task* g_run_queues[8] = {nullptr};
static Task* g_run_queue_tails[8] = {nullptr};

// Priority to queue index mapping
constexpr int priority_to_queue(Priority p) {
    switch (p) {
        case Priority::Realtime: return 0;
        case Priority::High:     return 1;
        case Priority::Normal:   return 2;
        case Priority::Low:      return 3;
        case Priority::Idle:     return 7;
        default:                 return 2;
    }
}

// Add task to run queue
static void enqueue_task(Task* task) {
    int queue = priority_to_queue(task->priority);
    task->next = nullptr;
    task->prev = g_run_queue_tails[queue];

    if (g_run_queue_tails[queue]) {
        g_run_queue_tails[queue]->next = task;
    } else {
        g_run_queues[queue] = task;
    }
    g_run_queue_tails[queue] = task;
}

// Remove task from run queue
static void dequeue_task(Task* task) {
    int queue = priority_to_queue(task->priority);

    if (task->prev) {
        task->prev->next = task->next;
    } else {
        g_run_queues[queue] = task->next;
    }

    if (task->next) {
        task->next->prev = task->prev;
    } else {
        g_run_queue_tails[queue] = task->prev;
    }

    task->next = task->prev = nullptr;
}

// Get next task to run
static Task* pick_next_task() {
    // Check queues in priority order
    for (int q = 0; q < 8; q++) {
        if (g_run_queues[q]) {
            Task* task = g_run_queues[q];
            dequeue_task(task);
            return task;
        }
    }
    return g_idle_task;
}

// Initialize scheduler
void init() {
    // Create idle task
    paddr_t idle_stack = memory::kernel_stack_alloc();
    arch::CpuContext idle_ctx = arch::CpuContext::new_kernel(
        reinterpret_cast<u64>(idle_task_entry),
        idle_stack
    );

    g_idle_task = new (memory::kmalloc(sizeof(Task))) Task();
    g_idle_task->id = 0;
    __builtin_strncpy(g_idle_task->name, "idle", 63);
    g_idle_task->state = TaskState::Ready;
    g_idle_task->priority = Priority::Idle;
    g_idle_task->context = idle_ctx;
    g_idle_task->kernel_stack = idle_stack;
    g_idle_task->user_stack = 0;
    g_idle_task->page_table = arch::paging::active_page_table();
    g_idle_task->parent = 0;
    g_idle_task->exit_code = 0;
    g_idle_task->sleep_until = 0;
    g_idle_task->next = g_idle_task->prev = nullptr;

    g_current_task = g_idle_task;
    g_tasks[0] = g_idle_task;
    g_task_count = 1;
}

// Get idle task
Task* get_idle_task() {
    return g_idle_task;
}

// Idle task entry point
extern "C" void idle_task_entry() {
    while (true) {
        arch::cpu::halt();
    }
}

// Spawn kernel thread
TaskId spawn_kernel(const char* name, void (*entry)(), Priority priority) {
    spinlock_guard(g_scheduler_lock);

    paddr_t stack = memory::kernel_stack_alloc();
    if (!stack) return 0;

    arch::CpuContext ctx = arch::CpuContext::new_kernel(
        reinterpret_cast<u64>(entry),
        stack
    );

    Task* task = new (memory::kmalloc(sizeof(Task))) Task();
    task->id = g_next_task_id++;
    __builtin_strncpy(task->name, name, 63);
    task->name[63] = 0;
    task->state = TaskState::Ready;
    task->priority = priority;
    task->context = ctx;
    task->kernel_stack = stack;
    task->user_stack = 0;
    task->page_table = arch::paging::active_page_table();
    task->parent = g_current_task ? g_current_task->id : 0;
    task->exit_code = 0;
    task->sleep_until = 0;
    task->next = task->prev = nullptr;

    g_tasks[g_task_count++] = task;
    enqueue_task(task);

    return task->id;
}

// Spawn user process
TaskId spawn_user(const char* name, u64 entry_point, u64 user_stack, Priority priority) {
    spinlock_guard(g_scheduler_lock);

    paddr_t stack = memory::kernel_stack_alloc();
    if (!stack) return 0;

    arch::paging::PageTable* page_table = arch::paging::create_page_table();
    if (!page_table) {
        memory::kernel_stack_free(stack);
        return 0;
    }

    arch::CpuContext ctx = arch::CpuContext::new_user(entry_point, user_stack, stack);

    Task* task = new (memory::kmalloc(sizeof(Task))) Task();
    task->id = g_next_task_id++;
    __builtin_strncpy(task->name, name, 63);
    task->name[63] = 0;
    task->state = TaskState::Ready;
    task->priority = priority;
    task->context = ctx;
    task->kernel_stack = stack;
    task->user_stack = user_stack;
    task->page_table = page_table;
    task->parent = g_current_task ? g_current_task->id : 0;
    task->exit_code = 0;
    task->sleep_until = 0;
    task->next = task->prev = nullptr;

    g_tasks[g_task_count++] = task;
    enqueue_task(task);

    return task->id;
}

// Get current task
Task* current_task() {
    return g_current_task;
}

// Set current task
void set_current_task(Task* task) {
    g_current_task = task;
}

// Get task by ID
Task* get_task(TaskId id) {
    if (id < g_task_count) {
        return g_tasks[id];
    }
    return nullptr;
}

// Yield current task
void yield() {
    spinlock_guard(g_scheduler_lock);

    if (g_current_task && g_current_task != g_idle_task) {
        if (g_current_task->state == TaskState::Running) {
            g_current_task->state = TaskState::Ready;
            enqueue_task(g_current_task);
        }
    }

    Task* next = pick_next_task();
    if (next != g_current_task) {
        Task* old = g_current_task;
        g_current_task = next;
        g_current_task->state = TaskState::Running;

        // Switch page table if needed
        if (old->page_table != next->page_table) {
            arch::paging::switch_page_table(next->page_table);
        }

        // Update TSS kernel stack for syscall entry
        arch::gdt::set_kernel_stack(next->context.general.rsp);

        context_switch(&old->context, &next->context);
    }
}

// Sleep current task
void sleep(u64 ticks) {
    spinlock_guard(g_scheduler_lock);

    if (g_current_task && g_current_task != g_idle_task) {
        g_current_task->state = TaskState::Sleeping;
        g_current_task->sleep_until = arch::time::ticks() + ticks;
        yield();
    }
}

// Timer tick handler (called from PIT interrupt)
extern "C" void task_timer_tick() {
    u64 now = arch::time::ticks();

    // Wake up sleeping tasks
    for (usize i = 0; i < g_task_count; i++) {
        Task* task = g_tasks[i];
        if (task && task->state == TaskState::Sleeping) {
            if (now >= task->sleep_until) {
                task->state = TaskState::Ready;
                task->sleep_until = 0;
                enqueue_task(task);
            }
        }
    }

    // Yield current task if it's not idle
    if (g_current_task && g_current_task != g_idle_task) {
        yield();
    }
}

// Public wrapper for timer tick
void timer_tick() {
    task_timer_tick();
}

// Exit current task
[[noreturn]] void exit(i32 code) {
    spinlock_guard(g_scheduler_lock);

    if (g_current_task && g_current_task != g_idle_task) {
        g_current_task->state = TaskState::Zombie;
        g_current_task->exit_code = code;

        // Free resources
        if (g_current_task->kernel_stack) {
            memory::kernel_stack_free(g_current_task->kernel_stack);
        }
        if (g_current_task->page_table &&
            g_current_task->page_table != arch::paging::active_page_table()) {
            // TODO: Free page table frames
        }

        // Remove from task array (mark as dead)
        // We don't actually remove to keep IDs stable
    }

    yield();
    arch::cpu::halt();  // Should never reach here
}

// Main scheduler loop
[[noreturn]] void schedule() {
    while (true) {
        yield();
    }
}

// Spawn init process
void spawn_init() {
    // In a real OS, this would load and execute /sbin/init
    arch::serial::write_str("Init process would be spawned here\n");
    arch::serial::write_str("Available tasks: ");
    arch::serial::write_dec(g_task_count);
    arch::serial::write_str("\n");
}

// Test kernel thread
void test_thread() {
    for (int i = 0; i < 5; i++) {
        arch::serial::write_str("Test thread running: ");
        arch::serial::write_dec(i);
        arch::serial::write_str("\n");
        task::sleep(100);  // Sleep ~100ms
    }
    task::exit(0);
}

} // namespace task