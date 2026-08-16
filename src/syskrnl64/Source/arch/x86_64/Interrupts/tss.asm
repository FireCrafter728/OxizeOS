; SPDX-License-Identifier: GPL-3.0-or-later

[bits 64]

section .text

; LoadTSS
;
; Input:
; RCX: TSS descriptor offset in the GDT
;
; Output: None;
; Loads the Task Switch Segment
global LoadTSS
LoadTSS:
	ltr cx
	ret