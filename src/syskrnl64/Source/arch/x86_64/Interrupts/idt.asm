; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]
default rel

section .code

; IDT_Load
;
; Input:
; RCX: Pointer to the IDT Descriptor
;
; Output: None
;
; Loads the IDT with the specified descriptor
global IDT_Load
IDT_Load:
	lidt [rcx]
	xor rax, rax
	ret