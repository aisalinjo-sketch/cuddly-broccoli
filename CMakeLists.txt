cmake_minimum_required(VERSION 3.20)
project(NovariaKernel LANGUAGES CXX ASM)

# Kernel version
set(NOVARIA_VERSION "0.1.0")
set(NOVARIA_VERSION_MAJOR 0)
set(NOVARIA_VERSION_MINOR 1)
set(NOVARIA_VERSION_PATCH 0)

# Target architecture
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler settings for freestanding kernel
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Freestanding flags - use add_compile_options for proper handling
add_compile_options(
    -ffreestanding
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -fno-use-cxa-atexit
    -fno-stack-protector
    -mno-red-zone
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mno-3dnow
    -mcmodel=kernel
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -O2
    -g
    -std=c++23
)

# ASM flags
set(CMAKE_ASM_FLAGS "-felf64")
set(CMAKE_ASM_NASM_FLAGS "-f elf64")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS
    "-nostdlib"
    "-static"
    "-z max-page-size=0x1000"
    "-T ${CMAKE_SOURCE_DIR}/kernel.ld"
)

# Enable ASM_NASM
enable_language(ASM_NASM)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
file(GLOB_RECURSE KERNEL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/**/*.cpp
)

file(GLOB_RECURSE KERNEL_ASM_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.asm
    ${CMAKE_SOURCE_DIR}/src/**/*.asm
)

# Create kernel executable
add_executable(novaria-kernel ${KERNEL_SOURCES} ${KERNEL_ASM_SOURCES})

# Set output name
set_target_properties(novaria-kernel PROPERTIES
    OUTPUT_NAME "kernel"
    SUFFIX ".elf"
)

# Custom target for creating bootable ISO (requires grub-mkrescue)
add_custom_target(iso
    COMMAND mkdir -p iso/boot/grub
    COMMAND cp ${CMAKE_BINARY_DIR}/kernel.elf iso/boot/kernel.elf
    COMMAND cp ${CMAKE_SOURCE_DIR}/grub.cfg iso/boot/grub/grub.cfg
    COMMAND grub-mkrescue -o ${CMAKE_BINARY_DIR}/novaria-${NOVARIA_VERSION}.iso iso
    DEPENDS novaria-kernel
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating bootable ISO"
)

# QEMU run target
add_custom_target(run
    COMMAND qemu-system-x86_64
        -cdrom ${CMAKE_BINARY_DIR}/novaria-${NOVARIA_VERSION}.iso
        -serial stdio
        -display default
        -bios /opt/homebrew/share/qemu/OVMF.fd
    DEPENDS iso
    COMMENT "Running kernel in QEMU"
)

# Debug target
add_custom_target(debug
    COMMAND qemu-system-x86_64
        -cdrom ${CMAKE_BINARY_DIR}/novaria-${NOVARIA_VERSION}.iso
        -serial stdio
        -display default
        -bios /opt/homebrew/share/qemu/OVMF.fd
        -s -S
    DEPENDS iso
    COMMENT "Running kernel in QEMU with GDB server"
)

# Clean target
add_custom_target(clean-all
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/*
    COMMAND rm -rf ${CMAKE_SOURCE_DIR}/iso
    COMMENT "Clean all build artifacts"
)