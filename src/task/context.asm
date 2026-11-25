; Context Switch Assembly for x86_64

bits 64
section .text

global context_switch
extern task_schedule_next

; void context_switch(CpuContext* old_ctx, CpuContext* new_ctx)
; RDI = old_ctx, RSI = new_ctx
context_switch:
    ; Save new_ctx pointer on stack (we'll need it after switching stacks)
    push rsi

    ; Save old context (RDI)
    mov [rdi + 0x00], rax
    mov [rdi + 0x08], rbx
    mov [rdi + 0x10], rcx
    mov [rdi + 0x18], rdx
    mov [rdi + 0x20], rsi
    mov [rdi + 0x28], rdi
    mov [rdi + 0x30], rbp
    mov [rdi + 0x38], rsp
    mov [rdi + 0x40], r8
    mov [rdi + 0x48], r9
    mov [rdi + 0x50], r10
    mov [rdi + 0x58], r11
    mov [rdi + 0x60], r12
    mov [rdi + 0x68], r13
    mov [rdi + 0x70], r14
    mov [rdi + 0x78], r15

    ; Save RIP (return address is on stack)
    mov rax, [rsp + 8]  ; Skip the saved new_ctx
    mov [rdi + 0x80], rax

    ; Save RFLAGS
    pushfq
    pop rax
    mov [rdi + 0x88], rax

    ; Save segment registers
    mov [rdi + 0x90], cs
    mov [rdi + 0x98], ss
    mov [rdi + 0xA0], ds
    mov [rdi + 0xA8], es
    mov [rdi + 0xB0], fs
    mov [rdi + 0xB8], gs

    ; Get new_ctx from stack
    pop rsi

    ; Switch to new stack
    mov rsp, [rsi + 0x38]

    ; Restore segment registers
    mov rax, [rsi + 0xB8]
    mov gs, ax
    mov rax, [rsi + 0xB0]
    mov fs, ax
    mov rax, [rsi + 0xA8]
    mov es, ax
    mov rax, [rsi + 0xA0]
    mov ds, ax
    mov rax, [rsi + 0x98]
    mov ss, ax
    mov rax, [rsi + 0x90]
    mov cs, ax  ; This doesn't work directly, need far jump/ret

    ; Restore RFLAGS
    mov rax, [rsi + 0x88]
    push rax
    popfq

    ; Restore general registers (except RSP which we already set)
    mov rax, [rsi + 0x00]
    mov rbx, [rsi + 0x08]
    mov rcx, [rsi + 0x10]
    mov rdx, [rsi + 0x18]
    mov rsi, [rsi + 0x20]
    mov rdi, [rsi + 0x28]
    mov rbp, [rsi + 0x30]
    ; RSP already set
    mov r8,  [rsi + 0x40]
    mov r9,  [rsi + 0x48]
    mov r10, [rsi + 0x50]
    mov r11, [rsi + 0x58]
    mov r12, [rsi + 0x60]
    mov r13, [rsi + 0x68]
    mov r14, [rsi + 0x70]
    mov r15, [rsi + 0x78]

    ; Restore RIP via ret
    mov rax, [rsi + 0x80]
    push rax
    ret