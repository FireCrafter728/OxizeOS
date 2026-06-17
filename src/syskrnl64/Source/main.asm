[bits 64]

section .text

; _start
;
; Input:
; RDI: Pointer to the system table
;
; Output: None
;
; Sets up the system for C++ and executes the C++ kernel_bootstrap() function, then kernel_main() function
extern kernel_bootstrap
extern kernel_main

extern __init_array_start
extern __init_array_end

global _start
_start:
    ; Store the System Table ptr
    mov [rel SysTablePtr], rdi

    ; Align the stack to 16 bytes for SIMD instructions which require 16 byte alignment
    and rsp, -16
    ; Construct global constructors

    lea rcx, [rel __init_array_end]
    lea rbx, [rel __init_array_start]

.init_loop:

    cmp rbx, rcx
    je .init_done

    mov rax, [rbx]
    add rbx, 8
    test rax, rax
    je .init_loop

    call rax
    jmp .init_loop

.init_done:
    ; Call the kernel bootstrap

    mov rdi, [rel SysTablePtr]

    call kernel_bootstrap
    ; rax should contain the pointer to the end of the new stack, store it inside rsp & rbp

    cli

    mov rsp, rax
    mov rbp, rsp

    mov rdi, [rel SysTablePtr]

    ; Call the kernel main function
    call kernel_main

    cli
    hlt

section .bss

SysTablePtr: resq 1