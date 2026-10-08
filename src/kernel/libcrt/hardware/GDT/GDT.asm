bits 64
default rel

section .text

global _LoadGDTR
;   (rax(bool))LoadGDT(rdi(void *))
_LoadGDTR:
    lgdt [rdi]
	xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

	; Get the return RIP (where LoadMinimalGDT was called from)
    pop rdx
	; The offset of your new Code Segment in the GDT
    mov rax, 0x08
	; Push new CS
    push rax
	; Push RIP
    push rdx
    mov rax, 1
    retfq

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