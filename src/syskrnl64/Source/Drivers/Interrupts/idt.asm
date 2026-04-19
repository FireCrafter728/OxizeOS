[bits 64]

section .text

; IDT_Load
;
; Input:
; RDI: Pointer to the IDT Descriptor
;
; Output: None
;
; Loads the IDT with the specified descriptor
global IDT_Load
IDT_Load:
    lidt [rdi]
    xor rax, rax
    ret