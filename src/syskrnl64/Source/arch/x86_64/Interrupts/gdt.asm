; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]
default rel

section .code

; GDT_Load
;
; Input:
; RCX: Pointer to the GDT Descriptor structure
; RDX: New code segment offset to use
; R8: New data segment offset to use
;
; Output: None
;
; Loads the GDT with the specified descriptor
global GDT_Load
GDT_Load:
	; Load the new GDT
	lgdt [rcx]

	; Perform a far jump to reload the code segment
	push rdx
	lea rax, [.reload_cs]
	push rax
	retfq

.reload_cs:
	; Update the other segment registers 
	mov rax, r8
	mov ds, ax
	mov es, ax
	mov ss, ax

	xor rax, rax
	ret