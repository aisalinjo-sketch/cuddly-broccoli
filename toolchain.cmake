# Toolchain file for x86_64-elf cross-compilation
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake ..

cmake_minimum_required(VERSION 3.20)

# Find cross-compiler (Linux: x86_64-elf-gcc, macOS: x86_64-elf-gcc from Homebrew)
find_program(X86_64_ELF_GCC x86_64-elf-gcc
    PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
)
find_program(X86_64_ELF_GXX x86_64-elf-g++
    PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
)
find_program(X86_64_ELF_LD x86_64-elf-ld
    PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
)
find_program(X86_64_ELF_OBJCOPY x86_64-elf-objcopy
    PATHS /opt/homebrew/bin /usr/local/bin /usr/bin
)

if(NOT X86_64_ELF_GCC OR NOT X86_64_ELF_GXX)
    message(FATAL_ERROR "x86_64-elf-gcc/g++ not found. Install: brew install x86_64-elf-gcc (macOS) or apt install x86-64-elf-gcc (Linux)")
endif()

# Set cross-compiler
set(CMAKE_C_COMPILER ${X86_64_ELF_GCC})
set(CMAKE_CXX_COMPILER ${X86_64_ELF_GXX})
set(CMAKE_LINKER ${X86_64_ELF_LD})
set(CMAKE_OBJCOPY ${X86_64_ELF_OBJCOPY})

# Target system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Force GNU compiler ID
set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_CXX_COMPILER_ID "GNU")
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

# Find nasm
find_program(NASM nasm)
if(NOT NASM)
    message(FATAL_ERROR "NASM not found. Please install nasm.")
endif()