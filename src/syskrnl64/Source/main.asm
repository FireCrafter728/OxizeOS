[bits 64]

; _start
;
; Input:
; RDI: Pointer to the system table
; RSI: Boolean value whether PCID is supported and CR4.PCIDE was enabled
;
; Output: None
;
; Sets up the system for C++ and executes the C++ main() function
extern main

extern __init_array_start
extern __init_array_end

global _start
_start:

    ; We know that the stack is page-aligned, so it's also 16-byte aligned
    ; Since `push` decrements RSP by 8, we need to adjust alignment
    ; by subtracting 8 from RSP to account for misalignment

    sub rsp, 8

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

    call main

halt:
    cli
    hlt