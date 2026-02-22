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