; SPDX-License-Identifier: GPL-3.0-or-later
;
; OxizeOS Operating System for the x86 amd64(x86_64) architecture
; Copyright (C) 2025-2026 FireCrafter728
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

[bits 64]

section .text

; HaltSystemImpl
;
; Input: None
;
; Output: None
;
; Halts the system
global HaltSystemImpl
HaltSystemImpl:
	cli
	hlt


; EnableSSE
;
; Input: None
;
; Output: None
;
; Enables the SSE instructions
global EnableSSE
EnableSSE:
	; Modify CR0
	mov rax, cr0
	and rax, ~(1 << 2) ; Clear bit 2 of CR0(Coprocessor Emulation Bit)
	or rax, (1 << 1) ; Set bit 1 of CR0(Monitor Coprocessor Bit)
	mov cr0, rax

	; Modify CR4
	mov rax, cr4
	or rax, (1 << 9) ; Set OSFXSR bit of CR4(bit 9)
	or rax, (1 << 10) ; Set OSXMMEXCPT bit of CR4(bit 10)
	mov cr4, rax

	; Setup x87 FPU to an initial state and setup MXCSR register

	fninit

	sub rsp, 16
	mov dword [rsp], 0x1F80
	ldmxcsr [rsp]
	add rsp, 16

	xor rax, rax
	ret

; RDTSC
;
; Input: None
;
; Output:
; RAX: Time Stamp Counter value
;
; Returns the Time Stamp Counter value
global RDTSC
RDTSC:
	lfence
	rdtsc
	shl rdx, 32
    or rax, rdx
    ret

; ExecuteKernel
;
; Input:
; RCX: Address of the kernel entry point
; RDX: Address of the system table
; R8: New CR3 value
; R9: Address to the new stack top
;
; Output: None
;
; Executes the kernel at RCX and passes the System table pointer at RDX
; Additionally sets up the new page tables and stack
global ExecuteKernel
ExecuteKernel:
	cli

	; Reload CR3
	mov cr3, r8
	
	; Reload stack
	mov rsp, r9

	; Load the kernel
	mov rdi, rdx
	jmp rcx
global ExecuteKernelEnd
ExecuteKernelEnd:

