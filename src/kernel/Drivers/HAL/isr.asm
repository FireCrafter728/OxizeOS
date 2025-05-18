[bits 32]

extern ISR_Handler

%macro ISR_NOERRCODE 1

global ISR%1:
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

%include "Drivers/HAL/gen_isr.inc"

isr_common:
    pusha

    xor eax, eax
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp

    call ISR_Handler
    add esp, 4

    pop eax
    mov ds, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa
    add esp, 8
    iret
