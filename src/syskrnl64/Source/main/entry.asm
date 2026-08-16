; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]
default rel

section .entry

; _start
;
; Input:
; RCX: Pointer to the system table
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
	mov [SysTablePtr], rcx

	; Allocate a 32-byte shadow space required by the Microsoft x64 ABI
	sub rsp, 32

	; Unlike the System V ABI, the Microsoft x64 ABI expects the stack to be 16-byte aligned, before a CALL instruction, which itself pushes a 8-byte return address to the stack. In the System V ABI, the stack must be 16-byte aligned AFTER a CALL instruction

	; Construct global constructors

	lea rcx, [__init_array_end]
	lea rbx, [__init_array_start]

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

	mov rcx, [SysTablePtr]

	call kernel_bootstrap
	; rax should contain the pointer to the end of the new stack, store it inside rsp & rbp

	cli

	mov rsp, rax
	mov rbp, rsp

	mov rcx, [SysTablePtr]
	mov rsi, rsp

	sub rsp, 32

	; Call the kernel main function
	call kernel_main

	cli
	hlt

section .bss

align 8
SysTablePtr: resq 1