bits 64
default rel

section .text

global _LoadIDTR
;   (rax(bool))LoadGDT(rdi(void *))
_LoadIDTR:
    lidt [rdi]
    mov rax, 1
    ret

global _ReadIDTR
;   (rax(bool))LoadGDT(rdi(void *))
_ReadIDTR:
    sidt [rdi]
    mov rax, 1
    ret

global __sti
__sti:
	sti
	ret

global __cli
__cli:
	cli
	ret