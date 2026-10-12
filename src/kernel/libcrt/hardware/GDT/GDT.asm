bits 64
default rel

section .text

global _LoadGDTR
;	bool _LoadGDTR(const void *gdtr_ptr [rdi])
_LoadGDTR:
    lgdt [rdi]
    ;	Reload data segment registers with kernel data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

	push qword 0x8
	lea rax, [rel .Reload]
	push rax
	retfq
.Reload:
    mov rax, 1
    ret

global _ReadGDTR
; bool _ReadGDTR(void *gdtr_out [rdi])
_ReadGDTR:
    sgdt [rdi]
    mov rax, 1
    ret

global _LoadLDTR
; bool _LoadLDTR(uint16_t selector [di])
_LoadLDTR:
    lldt di             ; Load selector from register DI (do not dereference [rdi])
    mov rax, 1
    ret

global _ReadLDTR
; uint16_t _ReadLDTR(void)
_ReadLDTR:
    xor eax, eax
    sldt ax             ; Store LDT selector into AX
    ret