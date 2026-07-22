; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]
section .text

; ASM_GetLPDataForCurrentLP
;
; Input: None
;
; Output:
; RAX: Pointer to the LP Specific Data structure
;
; Returns the pointer to the current LP Specific data structure using the GS segment
global ASM_GetLPDataForCurrentLP
ASM_GetLPDataForCurrentLP:
    ; gs:0 should contain a pointer to it's parent structure, which is LPSpecificData*
    mov rax, qword [gs:0]
    ret