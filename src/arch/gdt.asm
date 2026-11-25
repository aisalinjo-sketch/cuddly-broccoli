; GDT Assembly Functions for x86_64

bits 64
section .text

global gdt_flush
global tss_flush

; void gdt_flush(u64 descriptor_ptr)
; Load GDT from descriptor pointer in RDI
gdt_flush:
    lgdt [rdi]
    ; Reload segment registers
    mov rax, 0x10        ; Kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; Reload CS with far jump
    push 0x08            ; Kernel code selector
    lea rax, [rel .flush_cs]
    push rax
    retfq
.flush_cs:
    ret

; void tss_flush(u16 selector)
; Load TSS selector in DI
tss_flush:
    ltr di
    ret