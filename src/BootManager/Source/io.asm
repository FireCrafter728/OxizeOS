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

	xor rax, rax
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

