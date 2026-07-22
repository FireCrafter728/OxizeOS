; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]

section .text

%macro ISR_NOERRCODE 1

global ISR%1
ISR%1:
    push 0
    push %1
    jmp isr_common

%endmacro

%macro ISR_ERRCODE 1

global ISR%1
ISR%1:
    push %1
    jmp isr_common

%endmacro

%include "Include/Drivers/Interrupts/gen_isrs.inc"

extern ISR_Handler

isr_common:
    ; Push all general-purpose registers
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rdx
    push rcx
    push rbx
    push rax
    mov rax, rsp
    push rax
    push rbp
    mov rax, es
    push rax
    mov rax, ds
    push rax

    ; change ds/es to 64-bit ring 0 data segment entries
    xor rax, rax
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; Call the C++ handler
    mov rdi, rsp
    sub rsp, 8 ; misalign the stack by 8
    call ISR_Handler

    add rsp, 8 ; undo the misalignment

    ; Restore the registers and restore ds/es
        
    pop rax
    mov ds, ax
    pop rax
    mov es, ax
    pop rbp
    add rsp, 8 ; Discard saved RSP: not needed
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; return

    add rsp, 16 ; pop the interrupt number and error code

    iretq