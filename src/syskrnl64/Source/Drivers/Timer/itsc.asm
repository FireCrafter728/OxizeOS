; SPDX-License-Identifier: GPL-3.0-or-later

; RDTSC
;
; Input: None
;
; Output:
; RAX: TSC Value
;
; Returns the value from the rdtsc instruction
global RDTSC
RDTSC:
    lfence
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret