bits 64
section .text

global LoadIDTR
;   (rax(bool))LoadGDT(rdi(void *))
LoadIDTR:
    lidt [rdi]
    mov rax, 1
    ret

global ReadIDTR
;   (rax(bool))LoadGDT(rdi(void *))
ReadIDTR:
    sidt [rdi]
    mov rax, 1
    ret