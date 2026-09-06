bits 64
section .text

global _LoadGDTR
;   (rax(bool))LoadGDT(rdi(void *))
_LoadGDTR:
    lgdt [rdi]
    mov rax, 1
    ret

global _ReadGDTR
;   (rax(bool))LoadGDT(rdi(void *))
_ReadGDTR:
    sgdt [rdi]
    mov rax, 1
    ret

global _LoadLDTR
;   (rax(bool))LoadGDT(rdi(void *))
_LoadLDTR:
    lldt [rdi]
    mov rax, 1
    ret

global _ReadLDTR
;   (rax(bool))LoadGDT(rdi(void *))
_ReadLDTR:
    sldt [rdi]
    mov rax, 1
    ret