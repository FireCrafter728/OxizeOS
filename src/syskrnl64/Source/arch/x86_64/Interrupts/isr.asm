; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]
default rel

section .code

%macro ISR_NOERRCODE 1

global ISR%1
ISR%1:
	push 0
	push %1
	jmp isr_common

%endmacro

%macro ISR_ERRCODE 1

global ISR%1
ISR%1:
	push %1
	jmp isr_common

%endmacro

%macro ISR_NOERRCODE_WITH_IST 1

global ISR%1
ISR%1:
	push 0
	push %1
	jmp ist_common

%endmacro

%macro ISR_ERRCODE_WITH_IST 1

global ISR%1
ISR%1:
	push %1
	jmp ist_common

%endmacro

%include "Include/arch/x86_64/Interrupts/gen_isrs.inc"

extern ISR_Handler

; LP-Specific data structure
; If the structure changes in the C++ header, this must be updated too

struc LPSpecificData
	.self: resq 1
	.identity: resd 6
	.ihStackTopPtr: resq 1
	.intDepth: resq 1
endstruc

; Collector function for regular ISRs

global isr_common
isr_common:
	; Check the LP Specific data to see if this interrupt is primary or nested
	; Then increment the interrupt nestedness level in the LP Specific data
	; If interrupt is primary, switch to the interrupt handler and create a stack frame
	; If interrupt is nested, use the existing stack and create a stack frame for the current interrupt

	; Save all regs modified
	push rax
	push rbx
	push rcx
	push rdx
	push rbp

	; Load the LP-Specific data into GS using the self ptr at the start of GS
	; So instead of using gs: to access each field, we can use a register + offset which is faster
	mov rbx, [gs:0]

	; Check if the interrupt is nested, skip the stack switch if it is
	mov rax, [rbx + LPSpecificData.intDepth]
	test rax, rax
	jnz .after_stack_check

	; Interrupt is primary, switch to the interrupt handler stack
	mov rcx, [rbx + LPSpecificData.ihStackTopPtr]

	; Before changing RSP move the start of the stack frame from the old stack to the new stack

	; Copy the registers that we saved to the new stack frame
	; The start of that frame is 96 bytes from the top of the stack
	mov rax, [rsp] ; Saved RBP
	mov [rcx - 96], rax

	mov rax, [rsp + 8] ; Saved RDX
	mov [rcx - 88], rax

	mov rax, [rsp + 16] ; Saved RCX
	mov [rcx - 80], rax

	mov rax, [rsp + 24] ; Saved RBX
	mov [rcx - 72], rax

	mov rax, [rsp + 32] ; Saved RAX
	mov [rcx - 64], rax

	; Copy over the interrupt number, error code, RIP, CS and RFLAGS, as those are guaranteed to be on the stack
	; The start of that frame is 56 bytes from the top of the stack
	mov rbp, rsp
	add rbp, 40 ; Account for the saved registers that were modified

	mov rax, [rbp] ; Interrupt number
	mov [rcx - 56], rax

	mov rax, [rbp + 8] ; Error code
	mov [rcx - 48], rax

	mov rax, [rbp + 16] ; RIP
	mov [rcx - 40], rax

	mov rax, [rbp + 24] ; CS
	mov [rcx - 32], rax

	mov rax, [rbp + 32] ; RFLAGS
	mov [rcx - 24], rax

	; Check if a CPU privilige level change happened during the interrupt call, as if it did, the CPU pushed 2 extra registers on the stack
	; The 2 low bits in the pushed CS register indicate the previous privilege level

	mov rdx, [rbp + 24]
	and rdx, 0x3
	test rdx, rdx
	jz .pad_extra_values

.store_extra_values:
	; Store the extra RSP and SS values the CPU has pushed
	mov rax, [rbp + 40] ; RSP
	mov [rcx - 16], rax

	mov rax, [rbp + 48] ; SS
	mov [rcx - 8], rax

	jmp .switch_stack

.pad_extra_values:
	; Store zeroes in the location where the RSP and SS values would normally be in
	mov [rcx - 16], qword  0
	mov [rcx - 8], qword 0

.switch_stack:
	; The stack frame start is moved over to the new stack
	; Before switching, we also need to store the old RSP to be able to switch back
	; Store the previous stack pointer in RBP
	; The MS x64 ABI marks RBP as a callee-saved register, meaning it must be the same before and after a call to a C++ function
	mov rbp, rsp
	sub rcx, 96
	mov rsp, rcx

.after_stack_check:
	; Increment LPSpecificData intDepth field
	mov rax, [rbx + LPSpecificData.intDepth]
	inc rax
	mov [rbx + LPSpecificData.intDepth], rax

	; The CPU pushed stack frame + RAX, RBX, RCX, RDX and RBP are already pushed, push the rest

	push rbp ; Push the kernel stack stored in RBP

	; Push the general purpose registers

	push rsi
	push rdi

	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	; Push data and extra segments

	mov rax, es
	push rax
	mov rax, ds
	push rax

	push 0 ; Push the handler flags

	; store the end of the stack frame in RBP

	mov rbp, rsp

	; change data and extra segments to 64-bit ring 0 data segment entries
	mov rax, 0x10
	mov ds, ax
	mov es, ax

	; Check if the current stack is 16-byte aligned
	; If not, push a copy of the flags with bit 0 set
	; For the actual flags, this bit should be reserved, but for the copy it's used to determine whether a copy of the flags exists to ensure that the original stack alignment is restored

	test rsp, 0xF
	jz .after_alignment_check

	; Stack is misaligned, push a copy of the flags with bit 0 set to realign the stack
	push 1

.after_alignment_check:

	; Call the C++ interrupt handler
	mov rcx, rbp
	sub rsp, 32 ; 32 bytes of shadow space required by the MS x64 ABI
	call ISR_Handler

	add rsp, 32 ; remove the shadow space

	; Check if the stack after the stack frame was aligned, if it was, restore the previous alignment

	pop rax
	cmp rax, 1
	jnz .after_restore

	add rsp, 8 ; Remove the actual flags

.after_restore:

	; Restore the stack frame

	; Restore data & extra segments
	pop rax
	mov ds, ax 
	pop rax
	mov es, ax

	; Restore the general purpose registers

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8

	pop rdi
	pop rsi
	
	pop rsp ; This is a special x86 case where the RSP is incremented before the value in the stack is written to the RSP register, which is useful in this case for restoring the old stack pointer from the stack frame

	; Decrement the intCount value in the LPSpecificData
	mov rbx, [gs:0]
	dec [rbx + LPSpecificData.intDepth]

	; In the old stack restore the 5 registers saved, which are rax, rbx, rcx, rdx and rbp

	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax

	add rsp, 16 ; pop the interrupt number and error code, as the iretq doesn't pop the error code itself

	; return

	iretq

; Collector function for interrupts that have an IST

global ist_common
ist_common:
	; push the rest of the registers to the stack frame
	; the CPU has already pushed:
	; SS and RSP of the previous CPL if the CPU changed privilege rings
	; RFLAGS, CS and RIP
	; Error code and Interrupt vector were pushed and padded by the entry stubs

	; Left to push:
	; RAX, RBX, RCX, RDX, RBP, RSP, RSI, RDI, R8-R15, ES, DS

	push rax
	push rbx
	push rcx
	push rdx

	push rbp
	push 0 ; Don't push the current RSP as it's pointless

	push rsi
	push rdi

	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov rax, es
	push rax
	mov rax, ds
	push rax

	push 0 ; Push the handler flags

	; Change the data segments to 64-bit ring 0 data segment
	mov ax, 0x10
	mov ds, ax
	mov es, ax

	; Store the stack frame in the first argument register

	mov rcx, rsp

	sub rsp, 40 ; 8 byte alignment and 32 byte shadow space required by the MS x64 abi

	call ISR_Handler

	add rsp, 40

	; Restore the stack frame

	add rsp, 8 ; Discard the handler flags

	pop rax
	mov ds, ax
	pop rax
	mov es, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8

	pop rdi
	pop rsi
	
	add rsp, 8 ; Skip the dummy RSP
	pop rbp

	pop rdx
	pop rcx
	pop rbx
	pop rax

	add rsp, 16 ; Discard the interrupt number and error code

	iretq