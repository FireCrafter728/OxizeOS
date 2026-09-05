; SPDX-License-Identifier: GPL-3.0-or-later

[bits 16]

section .code

%define STATUS_INITIALIZING 0xFFFF
%define STATUS_CPUID_UNSUPPORTED 0xF001
%define STATUS_LONG_MODE_UNSUPPORTED 0xF002
%define STATUS_BOOTSTRAP_COMPLETE 0xFF00
%define STATUS_EXECUTE_CXX_HANDLER 0xFF01

struc LPStartupHeaders
	.StartupStackSegment: resw 1
	.StartupStackOffset: resw 1
	.LPID: resd 1
	.CR3Value: resq 1
	.BootstrapStackTop: resq 1
	.SystemTablePtr: resq 1
	.CXXHandlerPtr: resq 1
	.LPInitComponents: resq 1
	.StatusCode: resw 1
endstruc

global LPStartupFuncStart
LPStartupFuncStart:

; LPStartupFunc
;
; Input: None
;
; Output: None
;
; Startup trampoline for an AP after the Startup IPI was sent

LPStartupFunc:
	jmp short LPStartup_AfterHeaders

align 8

global LPStartupHeadersInstance
LPStartupHeadersInstance:
	istruc LPStartupHeaders
	iend

LPStartup_AfterHeaders:
	mov si, word 0 ; Placeholder value for the header location, replace after relocation, offset is LPStartupHeadersInstance + sizeof(LPStartupHeaders) + 1
	mov ax, word 0 ; Placeholder value for the header location, replace after relocation, offset is LPStartupHeadersInstance + sizeof(LPStartupHeaders) + 4
	mov ds, ax

	; Disable interrupts and clear direction flag
	cli
	cld

	; Load the startup stack
	mov ss, [ds:si + LPStartupHeaders.StartupStackSegment]
	mov sp, [ds:si + LPStartupHeaders.StartupStackOffset]

	; Calculate the startup trampoline page-aligned adress and store it in es:di

	; Convert the segoff address in ds:si to 32-bit linear
	mov ebx, ds
	shl ebx, 4
	movzx esi, si
	add ebx, esi

	; page align the linear address
	and ebx, 0xFFFFF000

	; do not modify ebx, as it contains the trampoline address
	; also do not modify ds:si, as it contains the seg:off to the trampoline header

	; Check for CPUID support

	pushfd
	pop eax

	mov ecx, eax
	xor eax, 1 << 21

	push eax
	popfd

	pushfd
	pop eax

	xor eax, ecx
	test eax, 1 << 21
	jnz .cpuidSupported

.cpuidUnsupported:
	mov word [ds:si + LPStartupHeaders.StatusCode], STATUS_CPUID_UNSUPPORTED
	hlt

.cpuidSupported:
	; Check for long mode support in CPUID

	push ebx ; Save EBX as cpuid overwrites it

	mov eax, 0x80000001
	cpuid

	pop ebx

	test edx, 1 << 29
	jnz .longModeSupported

.longModeUnsupported:
	mov word [ds:si + LPStartupHeaders.StatusCode], STATUS_LONG_MODE_UNSUPPORTED
	hlt

.longModeSupported:
	jmp short .LoadTmpGDT

.TmpGDT:
	dq 0 ; NULL entry

	; 32-bit ring 0 code entry

	dw 0xFFFF ; limit low is 0xFFFF
	dw 0x0000 ; base low is 0
	db 0x00 ; base middle is 0
	db 0b10011010 ; Readable, non-conforming, executable, code segment, ring 0, present
	db 0b11001111 ; limit high is 0xF, 32-bit, granularity in 4K pages
	db 0 ; base high is 0

	; 32-bit ring 0 data entry

	dw 0xFFFF ; limit low is 0xFFFF
	dw 0x0000 ; base low is 0
	db 0x00 ; base middle is 0
	db 0b10010010 ; Readable, non-conforming, non-executable, data segment, ring 0, present
	db 0b11001111 ; limit high is 0xF, 32-bit, granularity in 4K pages
	db 0 ; base high is 0

	; 64-bit ring 0 code entry

	dw 0xFFFF ; limit low is 0xFFFF
	dw 0x0000 ; base low is 0
	db 0x00 ; base middle is 0
	db 0b10011010 ; Readable, non-conforming, executable, code segment, ring 0, present
	db 0b10101111 ; limit high is 0xF, 64-bit, granularity in 4K pages
	db 0 ; base high is 0

	; 64-bit ring 0 data entry

	dw 0xFFFF ; limit low is 0xFFFF
	dw 0x0000 ; base low is 0
	db 0x00 ; base middle is 0
	db 0b10010010 ; Readable, non-conforming, non-executable, data segment, ring 0, present
	db 0b10101111 ; limit high is 0xF, 64-bit, granularity in 4K pages
	db 0 ; base high is 0

.TmpGDTEnd:

.GDTDesc:
	dw .TmpGDTEnd - .TmpGDT - 1
	dq 0

.LoadTmpGDT:
	; Store the relocated TmpGDT address into GDTDesc from the startup header
	; EBX contains the page-aligned address of the trampoline, use it and the offset in the trampoline of .TmpGDT to store the absolute address in the GDT Descriptor

	mov eax, ebx
	add eax, .TmpGDT - LPStartupFuncStart
	mov cx, ds
	mov es, cx
	xor di, di
	add di, .GDTDesc - LPStartupFuncStart
	mov [es:di + 2], eax

	lgdt [es:di]

	; Enable the A20 gate

.EnableA20:
	; disable the keyboard
	call .A20WaitInput
	mov al, KbdControllerDisableKeyboard
	out KbdControllerCommandPort, al

	; read control output port
	call .A20WaitInput
	mov al, KbdControllerReadCtrlOutputPort
	out KbdControllerCommandPort, al

	call .A20WaitOutput
	in al, KbdControllerDataPort
	push eax

	; write control output port
	call .A20WaitInput
	mov al, KbdControllerWriteCtrlOutputPort
	out KbdControllerCommandPort, al

	call .A20WaitInput
	pop eax
	or al, 2 ; bit 2 = A20 enable bit
	out KbdControllerDataPort, al

	; enable the keyboard
	call .A20WaitInput
	mov al, KbdControllerEnableKeyboard
	out KbdControllerCommandPort, al

	call .A20WaitInput

	; Calculate linear address from stack segment and offset values in the headers
	movzx eax, word [ds:si + LPStartupHeaders.StartupStackSegment]
	shl eax, 4
	movzx edx, word [ds:si + LPStartupHeaders.StartupStackOffset]
	add eax, edx
	mov edi, eax

	; Transform segoff address in ds:si to linear in esi
	mov ax, ds
	movzx eax, ax
	shl eax, 4
	add eax, esi
	mov esi, eax

	; set Protection Enable bit in CR0

	mov eax, cr0
	or eax, 1
	mov cr0, eax

	; Jump into 32-bit protected mode

	push 0x08 ; 32-bit code segment selector
	mov eax, ebx
	add eax, .ProtectedModeEntry - LPStartupFuncStart
	push eax
	o32 retf ; far return to 0x08:.ProtectedModeEntry, o32 specifies to consume 4 bytes for the EIP to ensure the full linear address is used

.A20WaitInput:
	in al, KbdControllerCommandPort
	test al, 2
	jnz .A20WaitInput
	ret

.A20WaitOutput:
	in al, KbdControllerCommandPort
	test al, 1
	jz .A20WaitOutput
	ret

KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1
	
.ProtectedModeEntry:
	[bits 32]

	; Setup data segments

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax

	; Load the linear stack address saved in EDI

	mov esp, edi

	; Enable Physical Address Extensions in CR4

	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	; Load CR3
	; Can't load all 64-bits because protected mode doesn't have 64-bit registers, so will load only the low 32-bits and hope that the PML4 address is below 4GiB or whatever the limit is, which it should be, unless the firmware itself reserves gigabytes of physical address space below 4GiB, which it normally shouldn't

	mov eax, [esi + LPStartupHeaders.CR3Value]
	mov cr3, eax

	; Set Long Mode Enable in the EFER MSR
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; Set Paging Enable bit in CR0

	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	; Jump to the 64-bit mode temporary entry

	push 0x18
	mov eax, ebx
	add eax, .TmpLongModeEntry - LPStartupFuncStart
	push dword eax
	retf ; far return to 0x18:.TmpLongModeEntry

.TmpLongModeEntry:
	[bits 64]

	; Setup data segments
	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov ss, ax

	; Enable No-Execute bit in EFER MSR

	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 11
	wrmsr

	; Write the full 64-bits of the CR3 value in the header to the CR3 register
	mov rax, [rsi + LPStartupHeaders.CR3Value]
	mov cr3, rax

	; Jump to the final 64-bit mode entry
	push 0x18
	mov rax, rbx
	add rax, .LongModeEntry - LPStartupFuncStart
	push rax
	retfq

.LongModeEntry:
	; Load the bootstrap stack

	mov rsp, [rsi + LPStartupHeaders.BootstrapStackTop]

	; Enable SSE2

	mov rax, cr0
	and rax, ~(1 << 2) ; Clear bit 2 of CR0(Coprocessor Emulation Bit)
	or rax, (1 << 1) ; Set bit 1 of CR0(Monitor Coprocessor Bit)
	mov cr0, rax

	; Modify CR4
	mov rax, cr4
	or rax, (1 << 9) ; Set OSFXSR bit of CR4(bit 9)
	or rax, (1 << 10) ; Set OSXMMEXCPT bit of CR4(bit 10)
	mov cr4, rax

	; Setup x87 FPU to an initial state and setup MXCSR register

	fninit

	sub rsp, 16
	mov dword [rsp], 0x1F80
	ldmxcsr [rsp]
	add rsp, 16

	; Set the status to BOOTSTRAP_COMPLETE

	mov word [rsi + LPStartupHeaders.StatusCode], STATUS_BOOTSTRAP_COMPLETE

	; Wait until status is STATUS_EXECUTE_CXX_HANDLER

.waitForContinue:
	mov ax, [rsi + LPStartupHeaders.StatusCode]
	cmp ax, STATUS_EXECUTE_CXX_HANDLER
	jz .goToCXXHandler
	pause
	jmp .waitForContinue

.goToCXXHandler:

	; Load the system table ptr to RCX as the first argument to the C++ handler

	mov rcx, [rsi + LPStartupHeaders.SystemTablePtr]

	; Load the LPID value to RDX as the second argument to the C++ handler
	
	mov edx, [rsi + LPStartupHeaders.LPID]

	; Load the LPInitComponents ptr to R8 as the third argument to the C++ handler

	mov r8, [rsi + LPStartupHeaders.LPInitComponents]

	; Load the Status Code ptr to R9 as the fourth argument to the C++ handler

	lea r9, [rsi + LPStartupHeaders.StatusCode]

	; 32 byte shadow space for the MS x64 ABI
	sub rsp, 32

	; Load the C++ handler ptr to RAX
	mov rax, [rsi + LPStartupHeaders.CXXHandlerPtr]

	; Call the C++ handler
	call rax

	; If the C++ handler exited, halt the system
	cli
	hlt

LPStartupCodeEnd:

%if LPStartupCodeEnd - LPStartupFuncStart > 0x800
	%error "LP Startup trampoline code exceeds a maximum size of 2KiB"
%endif

; Temporary stack label, comes after 2KiB of the bootstrap page

align 2048

.StartupStack:
	times 2048 db 0

global LPStartupFuncEnd
LPStartupFuncEnd:

