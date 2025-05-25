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