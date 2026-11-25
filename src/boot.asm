; Novaria OS Kernel - x86_64 Assembly Bootstrap
; This is the entry point that sets up the environment for C++

bits 64
default rel
section .multiboot

; Multiboot 2 header
ALIGN 8
mb2_header_start:
    dd 0xE85250D6                ; magic number
    dd 0                          ; architecture (0 = i386/x86_64)
    dd mb2_header_end - mb2_header_start  ; header length
    dd 0x100000000 - (0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))  ; checksum

    ; End tag
    dw 0                          ; type
    dw 0                          ; flags
    dd 8                          ; size
mb2_header_end:

section .text
global _start
extern kernel_main

_start:
    ; The bootloader (GRUB) has already:
    ; - Switched to 64-bit mode
    ; - Set up a basic page table (identity mapping for low memory)
    ; - Loaded the kernel at 1MB
    ; - Provided multiboot info in RDI (physical address)

    ; Save multiboot info pointer (passed in RDI by GRUB)
    mov [multiboot_info_ptr], rdi

    ; Zero out BSS section
    extern bss_start, bss_end
    mov rdi, bss_start
    mov rcx, bss_end
    sub rcx, rdi
    xor rax, rax
    cld
    rep stosb

    ; Set up initial stack (boot_stack_top is defined in linker script)
    extern boot_stack_top
    mov rsp, boot_stack_top

    ; Call C++ kernel main
    call kernel_main

    ; If kernel_main returns, halt
    cli
.halt:
    hlt
    jmp .halt

section .bss
global multiboot_info_ptr
multiboot_info_ptr: resq 1

extern bss_start
extern bss_end