[bits 64]

section .text

; GDT_Load
;
; Input:
; RDI: Pointer to the GDT Descriptor structure
; RSI: New code segment offset to use
; RDX: New data segment offset to use
;
; Output: None
;
; Loads the GDT with the specified descriptor
global GDT_Load
GDT_Load:
    ; Load the new GDT
    lgdt [rdi]

    ; Perform a far jump to reload the code segment
    push rsi
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    ; Update the other segment registers 
    mov rax, rdx
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    xor rax, rax
    ret