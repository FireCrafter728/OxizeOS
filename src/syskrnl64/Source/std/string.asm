[bits 64]

section .text

; memset
;
; Input:
; RDI: Address of the region to memset
; RSI: Value to fill the region to
; RCX: How many times to write value
;
; Output:
; RAX: Address of the region start
;
; Fills a region with a specific value
global memset
memset:
    mov rax, rdi
    test rcx, rcx
    jz .memset_done
    xor rdx, rdx
    mov dl, sil
    movq xmm0, rdx
    pshufd xmm0, xmm0, 0
    movdqa xmm1, xmm0
    punpcklbw xmm0, xmm1
    punpckhbw xmm1, xmm1
    por xmm0, xmm1

.memset_align:
    test rdi, 0xF
    jz .aligned_loop
    mov byte [rdi], sil
    inc rdi
    dec rcx
    jz .memset_done
    jmp .memset_align

.aligned_loop:
    cmp rcx, 16
    jb .tail_bytes

.main_loop:
    movdqa [rdi], xmm0
    add rdi, 16
    sub rcx, 16
    cmp rcx, 16
    jae .main_loop

.tail_bytes:
    test rcx, rcx
    jz .memset_done

.tail_loop:
    mov byte [rdi], sil
    inc rdi
    dec rcx
    jnz .tail_loop

.memset_done:
    ret