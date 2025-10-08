[bits 64]

section .text
global HaltSystemImpl
HaltSystemImpl:
    cli
    hlt

global inb
inb:
    xor rax, rax
    mov dx, cx
    in al, dx
    ret

global outb
outb:
    mov al, dl
    mov dx, cx
    out dx, al
    ret

global inw
inw:
    xor rax, rax
    mov dx, cx
    in ax, dx
    ret
    
global outw
outw:
    mov ax, dx
    mov dx, cx
    out dx, ax
    ret

global ind
ind:
    xor rax, rax
    mov dx, cx
    in eax, dx
    ret
    
global outd
outd:
    mov eax, edx
    mov dx, cx
    out dx, eax
    ret