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
    out dx, eax
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


; uint64_t ASMCALL rdmsr(uint32_t msr);
global rdmsr
rdmsr:
    push ebp
    mov ebp, esp

    mov ecx, [ebp + 8]
    
    rdmsr

    mov esp, ebp
    pop ebp
    ret


; void ASMCALL wrmsr(uint32_t msr, uint64_t value);
global wrmsr
wrmsr:
    push ebp
    mov ebp, esp

    mov ecx, [ebp + 8]
    mov eax, [ebp + 12]
    mov edx, [ebp + 16]

    wrmsr

    mov esp, ebp
    pop ebp
    ret

;void ASMCALL sleep(uint32_t iterations);
global sleep
sleep:
    push ebp
    mov ebp, esp

    mov ecx, [ebp + 8]

.loop:
    cmp ecx, 0
    je .done

    pause

    dec ecx
    jmp .loop

.done:
    mov esp, ebp
    pop ebp
    ret