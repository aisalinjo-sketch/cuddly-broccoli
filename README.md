# Novaria OS Kernel (C++ Version)

A hobby operating system kernel written in C++23 for x86_64 UEFI systems.

This is a C++ port of the original Rust-based Novaria kernel, maintaining the same architecture and features.

## Architecture

- **Language**: C++23 (freestanding)
- **Target**: x86_64 (UEFI/BIOS via GRUB multiboot2)
- **Build System**: CMake + NASM
- **License**: MIT OR Apache-2.0

## Project Structure

```
kernel-cpp/
├── CMakeLists.txt          # CMake build configuration
├── kernel.ld               # Linker script
├── grub.cfg                # GRUB configuration
├── build.sh                # Build script
├── run.sh                  # Run in QEMU
├── debug.sh                # Debug with GDB
└── src/
    ├── boot.asm            # Assembly bootstrap (multiboot2)
    ├── main.cpp            # Kernel entry point
    ├── common.hpp          # Common types (Result, Option, SpinLock, etc.)
    ├── log.hpp             # Logging facility
    ├── panic.cpp           # Panic handler
    ├── arch.hpp            # Architecture interface
    ├── arch/
    │   ├── init.cpp        # Architecture initialization
    │   ├── gdt.cpp         # Global Descriptor Table
    │   ├── gdt.asm         # GDT assembly functions
    │   ├── idt.cpp         # Interrupt Descriptor Table
    │   ├── idt.asm         # IDT assembly stubs
    │   ├── paging.cpp      # Page table management
    │   ├── registers.hpp   # CPU register structures
    │   ├── serial.cpp      # Serial port (16550 UART)
    │   ├── time.cpp        # PIT, RTC, TSC timers
    │   ├── cpu.cpp         # CPU control functions
    │   ├── pic.cpp         # Programmable Interrupt Controller
    │   └── pic.hpp         # PIC interface
    ├── memory/
    │   ├── frame_allocator.cpp  # Physical frame allocator (bitmap)
    │   ├── frame_allocator.hpp  # Frame allocator interface
    │   ├── heap.cpp             # Kernel heap allocator
    │   └── init.cpp             # Memory initialization
    ├── task/
    │   ├── scheduler.cpp  # Priority-based round-robin scheduler
    │   ├── syscall.cpp    # System call interface
    │   ├── context.asm    # Context switching assembly
    │   └── init.cpp       # Task initialization
    ├── drivers/
    │   └── init.cpp       # Device driver initialization
    └── ui/
        └── init.cpp       # UI/console initialization
```

## Features Implemented

### Core Kernel
- ✅ Multiboot2 boot with GRUB
- ✅ GDT with kernel/user segments and TSS
- ✅ IDT with exception and hardware interrupt handlers
- ✅ Paging (4-level page tables, kernel/user mappings)
- ✅ Physical frame allocator (bitmap-based)
- ✅ Kernel heap allocator (bump + free list)
- ✅ Priority-based round-robin scheduler
- ✅ Process/thread management with context switching
- ✅ System call interface (stubs for 40+ syscalls)

### Device Drivers
- ✅ Serial port (16550 UART, COM1)
- ✅ PIT timer (1ms resolution)
- ✅ RTC (date/time)
- ✅ TSC calibration
- ✅ PIC (8259) interrupt controller

### User Interface
- ✅ Serial logging (custom log crate-like backend)
- ✅ Basic console output

## Building

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake nasm qemu-system-x86_64 grub2-common xorriso ovmf

# Arch Linux
sudo pacman -S base-devel cmake nasm qemu-system-x86_64 grub ovmf

# macOS (with Homebrew)
brew install cmake nasm qemu grub
```

### Build Kernel

```bash
cd kernel-cpp
./build.sh
```

This creates:
- `build/novaria-kernel.elf` - Kernel ELF executable
- `build/novaria-0.1.0.iso` - Bootable ISO image (if grub-mkrescue available)

### Run in QEMU

```bash
./run.sh
```

### Debug with GDB

```bash
# Terminal 1: Start QEMU with GDB server
./debug.sh

# Terminal 2: Connect with GDB
gdb build/novaria-kernel.elf
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

## Kernel Commands

Once booted, the kernel will:
1. Initialize all subsystems
2. Spawn a test thread that prints messages
3. Enter the scheduler loop

Output appears on the serial console (QEMU stdio).

## Development

### Adding a New System Call

1. Add variant to `Syscall` enum in `src/task/syscall.cpp`
2. Implement handler function
3. Add match arm in `dispatch()`

### Memory Management

- Physical frames: `memory::allocate_frame()`, `memory::deallocate_frame()`
- Kernel heap: `memory::kmalloc()`, `memory::kfree()`, `memory::krealloc()`
- Virtual mapping: `arch::paging::map_page()`, `arch::paging::map_range()`

### Task Management

- Spawn kernel thread: `task::spawn_kernel(name, entry, priority)`
- Spawn user process: `task::spawn_user(name, entry_point, user_stack, priority)`
- Yield: `task::yield()`
- Sleep: `task::sleep(ticks)`

## Architecture Details

### Memory Layout

```
0x0000_0000_0000_0000 - 0x0000_7FFF_FFFF_FFFF  User space (128 TB)
0x0000_8000_0000_0000 - 0xFFFF_7FFF_FFFF_FFFF  Unmapped (guard)
0xFFFF_8000_0000_0000 - 0xFFFF_FFFF_FFFF_FFFF  Kernel space (128 TB)
```

Kernel linked at 1MB (0x100000) with higher-half direct mapping at 0xFFFF800000000000.

### Interrupt Vectors

- 0-31: CPU exceptions
- 32-47: Hardware IRQs (PIC)
- 0x80: System call (int 0x80)

### Calling Convention

System calls use x86_64 System V ABI:
- Syscall number in RAX
- Arguments in RDI, RSI, RDX, R10, R8, R9
- Return value in RAX

## License

MIT OR Apache-2.0