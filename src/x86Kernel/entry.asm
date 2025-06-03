bits 32
section .entry
global _start
extern _x86kernel

extern run_global_constructors

_start:
    cli

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0xFFF0
    xor ebp, ebp

    mov eax, run_global_constructors
    
    call eax
    
    call _x86kernel

    hlt