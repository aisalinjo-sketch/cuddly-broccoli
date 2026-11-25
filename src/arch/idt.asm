; IDT Assembly Stubs for x86_64
; Provides interrupt/exception entry points that save context and call C handlers

bits 64
section .text

; Macro to generate ISR stub for exceptions (with error code)
%macro ISR_ERRCODE 1
global isr_stub_%1
isr_stub_%1:
    ; Push error code (already on stack from CPU)
    ; Push vector number
    push qword %1
    jmp isr_common
%endmacro

; Macro to generate ISR stub for exceptions (without error code)
%macro ISR_NOERRCODE 1
global isr_stub_%1
isr_stub_%1:
    ; Push dummy error code (0)
    push qword 0
    ; Push vector number
    push qword %1
    jmp isr_common
%endmacro

; Macro to generate IRQ stub
%macro IRQ_STUB 1
global irq_stub_%1
irq_stub_%1:
    ; Push dummy error code (0)
    push qword 0
    ; Push vector number (32 + IRQ)
    push qword %1 + 32
    jmp irq_common
%endmacro

; ISR stubs for exceptions 0-31
ISR_NOERRCODE 0   ; Division by zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; NMI
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Overflow
ISR_NOERRCODE 5   ; Bound range exceeded
ISR_NOERRCODE 6   ; Invalid opcode
ISR_NOERRCODE 7   ; Device not available
ISR_ERRCODE   8   ; Double fault
ISR_NOERRCODE 9   ; Coprocessor segment overrun
ISR_ERRCODE   10  ; Invalid TSS
ISR_ERRCODE   11  ; Segment not present
ISR_ERRCODE   12  ; Stack segment fault
ISR_ERRCODE   13  ; General protection fault
ISR_ERRCODE   14  ; Page fault
ISR_NOERRCODE 15  ; Reserved
ISR_NOERRCODE 16  ; x87 FPU error
ISR_ERRCODE   17  ; Alignment check
ISR_NOERRCODE 18  ; Machine check
ISR_NOERRCODE 19  ; SIMD FP exception
ISR_NOERRCODE 20  ; Virtualization exception
ISR_NOERRCODE 21  ; Control protection exception
ISR_NOERRCODE 22  ; Reserved
ISR_NOERRCODE 23  ; Reserved
ISR_NOERRCODE 24  ; Reserved
ISR_NOERRCODE 25  ; Reserved
ISR_NOERRCODE 26  ; Reserved
ISR_NOERRCODE 27  ; Reserved
ISR_NOERRCODE 28  ; Reserved
ISR_NOERRCODE 29  ; Reserved
ISR_NOERRCODE 30  ; Reserved
ISR_NOERRCODE 31  ; Reserved

; System call stub (vector 0x80, but we use index 32 in stub array)
global isr_stub_32
isr_stub_32:
    ; Syscall uses different calling convention
    ; Save user registers
    push qword 0        ; Dummy error code
    push qword 0x80     ; Vector 0x80
    jmp syscall_common

; IRQ stubs for IRQ 0-15
IRQ_STUB 0
IRQ_STUB 1
IRQ_STUB 2
IRQ_STUB 3
IRQ_STUB 4
IRQ_STUB 5
IRQ_STUB 6
IRQ_STUB 7
IRQ_STUB 8
IRQ_STUB 9
IRQ_STUB 10
IRQ_STUB 11
IRQ_STUB 12
IRQ_STUB 13
IRQ_STUB 14
IRQ_STUB 15

; Array of ISR stub pointers (for C code)
global isr_stubs
isr_stubs:
    dq isr_stub_0
    dq isr_stub_1
    dq isr_stub_2
    dq isr_stub_3
    dq isr_stub_4
    dq isr_stub_5
    dq isr_stub_6
    dq isr_stub_7
    dq isr_stub_8
    dq isr_stub_9
    dq isr_stub_10
    dq isr_stub_11
    dq isr_stub_12
    dq isr_stub_13
    dq isr_stub_14
    dq isr_stub_15
    dq isr_stub_16
    dq isr_stub_17
    dq isr_stub_18
    dq isr_stub_19
    dq isr_stub_20
    dq isr_stub_21
    dq isr_stub_22
    dq isr_stub_23
    dq isr_stub_24
    dq isr_stub_25
    dq isr_stub_26
    dq isr_stub_27
    dq isr_stub_28
    dq isr_stub_29
    dq isr_stub_30
    dq isr_stub_31
    dq isr_stub_32  ; Syscall

; Array of IRQ stub pointers
global irq_stubs
irq_stubs:
    dq irq_stub_0
    dq irq_stub_1
    dq irq_stub_2
    dq irq_stub_3
    dq irq_stub_4
    dq irq_stub_5
    dq irq_stub_6
    dq irq_stub_7
    dq irq_stub_8
    dq irq_stub_9
    dq irq_stub_10
    dq irq_stub_11
    dq irq_stub_12
    dq irq_stub_13
    dq irq_stub_14
    dq irq_stub_15

; Common ISR handler
; Stack layout on entry:
; [RSP] = vector number
; [RSP+8] = error code
; Then we push all registers
extern arch_idt_default_exception_handler
extern arch_idt_syscall_handler

isr_common:
    ; Save all general purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Save segment registers
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax

    ; RSP now points to InterruptFrame structure
    mov rdi, rsp        ; First arg: frame pointer
    mov rsi, [rsp + 16*8]  ; Second arg: vector number (after 16 pushed registers)

    ; Call C handler
    call arch_idt_default_exception_handler

    ; Restore segment registers
    pop rax
    mov gs, ax
    pop rax
    mov fs, ax
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    ; Restore general purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Pop error code and vector
    add rsp, 16

    ; Return from interrupt
    iretq

; Common IRQ handler
extern arch_idt_default_irq_handler

irq_common:
    ; Save all general purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Save segment registers
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax

    ; RSP now points to InterruptFrame structure
    mov rdi, rsp        ; First arg: frame pointer
    mov rsi, [rsp + 16*8]  ; Second arg: vector number

    ; Call C handler
    call arch_idt_default_irq_handler

    ; Restore segment registers
    pop rax
    mov gs, ax
    pop rax
    mov fs, ax
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    ; Restore general purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Pop error code and vector
    add rsp, 16

    ; Return from interrupt
    iretq

; Syscall common handler
syscall_common:
    ; Syscall entry: RCX = RIP, R11 = RFLAGS
    ; We need to save user context and switch to kernel stack

    ; Swap GS to get kernel TSS (assuming KERNEL_GS_BASE is set)
    swapgs

    ; Save user RSP/RBP
    mov [rsp - 8], rsp   ; Save user RSP below current stack
    mov [rsp - 16], rbp  ; Save user RBP

    ; Switch to kernel stack (from TSS)
    mov rsp, [gs:0]      ; TSS.RSP0 at offset 0

    ; Now save user context on kernel stack
    push r11            ; User RFLAGS
    push rcx            ; User RIP
    push rax            ; User RAX (syscall number)
    push rdi
    push rsi
    push rdx
    push r10            ; User RCX (4th arg)
    push r8
    push r9

    ; Save segment registers
    mov rax, ds
    push rax
    mov rax, es
    push rax
    mov rax, fs
    push rax
    mov rax, gs
    push rax

    ; RDI = frame pointer
    mov rdi, rsp

    ; Call syscall handler
    call arch_idt_syscall_handler

    ; Restore segment registers
    pop rax
    mov gs, ax
    pop rax
    mov fs, ax
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    ; Restore registers
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    pop rax
    pop rcx   ; User RIP
    pop r11   ; User RFLAGS

    ; Restore user RSP/RBP
    mov rsp, [rsp]  ; User RSP was saved at bottom
    swapgs
    iretq