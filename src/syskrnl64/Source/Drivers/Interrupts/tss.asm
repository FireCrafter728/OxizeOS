; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]

section .text

; LoadTSS
;
; Input:
; RDI: TSS descriptor offset in the GDT
;
; Output: Non;
; Loads the Task Switch Segment
global LoadTSS
LoadTSS:
    ltr di
    ret