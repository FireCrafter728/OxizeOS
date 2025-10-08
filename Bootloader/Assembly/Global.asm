[bits 64]

global HaltSystemImpl

section .text
HaltSystemImpl:
    cli
    hlt