; SPDX-License-Identifier: GPL-3.0-or-later

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

; Pause
;
; Input: None
;
; Output: None
;
; executes the `pause` instruction to slow down the current core
global Pause
Pause:
	pause
	ret

; InvalidatePage
;
; Input:
; RDI: Virtual address of the page to invalidate
;
; Output: None
;
; Invalidates the page address in the processors TLB to match the updated page tables
global InvalidatePage
InvalidatePage:
	invlpg [rdi]
	xor rax, rax
	ret

; outb
;
; Input:
; RDI: Port index to write to
; RSI: Value to write to port
;
; Output: None
;
; Writes a specified 8-bit value to a specified port
global outb
outb:
	mov rdx, rdi
	mov rax, rsi
	out dx, al
	ret

; inb
;
; Input:
; RDI: Port index to read from
;
; Output:
; RAX: Value in specified port
;
; Returns a 8-bit value from a specified port
global inb
inb:
	mov rdx, rdi
	xor rax, rax
	in al, dx
	ret

; TestInt
;
; Input: None
;
; Output: None
;
; triggers a CPU exception
global TestInt
TestInt:
	int 0x06
	ret

; DisableInterrupts
;
; Input: None
;
; Output: None
;
; Clears the Interrupt Enable bit in RFLAGS using CLI instruction
global DisableInterrupts
DisableInterrupts:
	cli
	ret

; EnableInterrupts
;
; Input: None
;
; Output: None
;
; Sets the Interrupt Enable bit in RFLAGS using STI instruction
global EnableInterrupts
EnableInterrupts:
	sti
	ret

; GetCPUID
;
; Input:
; RDI: CPUID Leaf
; RSI: CPUID subLeaf
;
; Output:
; RDX: Pointer to a buffer to store output registers to, has to be at least 32 bytes
;
; Returns information from CPUID and specified leaf/subleaf to an output buffer
; Information is stored in this format:
; Register | Byte offset | Length in bytes
; RAX      | 0           | 8
; RBX      | 8           | 8
; RCX      | 16          | 8
; RDX      | 24          | 8
global GetCPUID
GetCPUID:
	; save modified registers that aren't already saved
	push rbx

	; Move buffer ptr from RDX to R10 as RDX will be overwritten

	mov r10, rdx

	; Move leaf to RAX, subleaf to RCX and call CPUID
	mov rax, rdi
	mov rcx, rsi
	cpuid

	; Results are stored in RAX, RBX, RCX & RDX. Store them into provided buffer
	mov [r10], rax
	mov [r10 + 8], rbx
	mov [r10 + 16], rcx
	mov [r10 + 24], rdx

	pop rbx

	ret

; RDMSR
;
; Input:
; RDI: MSR Index
;
; Output:
; RAX: Value at MSR
;
; Returns the MSR value from the specified index
global RDMSR
RDMSR:
	; RDMSR expects MSR index in RCX
	mov rcx, rdi
	rdmsr
	
	; Combine EAX:EDX(MSR value) into RAX
	shl rdx, 32
	or 	rax, rdx
	ret

; WRMSR
;
; Input:
; RDI: MSR Index
; RSI: 64-bit value
;
; Output: None
;
; Writes a value to an MSR specified by the MSR index
global WRMSR
WRMSR:
	; Move MSR Index to RCX
	mov rcx, rdi

	; Move low 32 bits from RSI to RAX
	mov eax, esi

	; Move high 32 bits from RSI to RDX
	shr rsi, 32
	mov edx, esi

	wrmsr
	ret

; PauseCurrentCore
;
; Input: None
;
; Output: None
;
; Halts the current core until an interrupt occurs
global PauseCurrentCore
PauseCurrentCore:
	sti
	hlt
	ret