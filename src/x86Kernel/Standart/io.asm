[bits 32]

global outb
outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global inb
inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global outw
outw:
    mov dx, [esp + 4]
    mov ax, [esp + 8]
    out dx, ax
    ret

global inw
inw:
    mov dx, [esp + 4]
    xor eax, eax
    in ax, dx
    ret

global outd
outd:
    mov dx, [esp + 4]
    mov eax, [esp + 8]
    in eax, dx
    ret

global ind
ind:
    mov dx, [esp + 4]
    xor eax, eax
    in eax, dx
    ret

;void __attribute__((cdecl)) Halt();
global Halt
Halt:
    hlt

;void __attribute__((cdecl)) CLI();
global CLI
CLI:
    cli
    ret

;void __attribute__((cdecl)) STI();
global STI
STI:
    sti
    ret

;void ASMCALL Interrupt();
global Interrupt
Interrupt:
    mov eax, 10
    mov ebx, 0  
    div ebx
    ret