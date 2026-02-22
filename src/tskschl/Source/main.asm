[bits 64]

; _start
;
; Input:
; RDI: Pointer to the system table
;
; Output: None
;
; Sets up the system for C++ and executes the C++ main() function
extern main
global _start
_start:
    ; Enable interrupts after CR3 & Stack switch
    sti

    ; We know that the stack is page-aligned, so it's also 16-byte aligned
    ; Since `push` decrements RSP by 8, we need to adjust alignment
    ; by subtracting 8 from RSP to account for misalignment
    ; Input argument(System table ptr) is already stored in the expected register(rdi)
    sub rsp, 8
    call main

halt:
    cli
    hlt