/* Novaria OS Kernel Linker Script for x86_64 UEFI/BIOS */

ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 1MB (above the traditional BIOS area) */
    . = 1M;

    /* Multiboot header (for BIOS boot compatibility) */
    .multiboot ALIGN(8) :
    {
        *(.multiboot)
    }

    /* Read-only sections */
    .text ALIGN(4K) :
    {
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.eh_frame .eh_frame.*)
    }

    /* Read-write sections */
    .data ALIGN(4K) :
    {
        *(.data .data.*)
        *(.bss .bss.*)
        *(COMMON)
    }

    /* Stack for early boot (before we set up proper stacks) */
    .boot_stack ALIGN(4K) :
    {
        . = ALIGN(16);
        boot_stack_bottom = .;
        . += 16K;  /* 16 KiB stack */
        boot_stack_top = .;
    }

    /* Heap for kernel allocations (will be expanded by memory manager) */
    .heap ALIGN(4K) :
    {
        . = ALIGN(4K);
        heap_start = .;
        . += 1M;  /* Initial 1 MiB heap */
        heap_end = .;
    }

    /* End marker */
    kernel_end = .;
}