[bits 32]

; void __attribute__((cdecl)) GDT_Load(GDTDescriptor* GDTDesc, uint16_t codeSeg, uint16_t dataSeg);
global GDT_Load
GDT_Load:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    lgdt [eax]

    mov eax, [ebp + 12]
    push eax
    push .reload_cs
    retf

.reload_cs:

    mov ax, [ebp + 16]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, ebp
    pop ebp
    ret