bits 32

extern isr_handlerC

isr_inthandle:
    call _int32disable
    pusha                       ; save general-purpose registers
    xor eax, eax
    mov ax, ds
    push eax

    mov ax, 0x10               ; kernel data segments
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp

	call isr_handlerC
	pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    add esp, 8                  ; Pop interrupt vector and error code
	popa
    call _int32enable
	iret

global _LoadIDT
;   void LoadIDT(idtDESC_t *)
_LoadIDT:
    call _int32disable
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]   ; Not sure if correct
    lidt [eax]
    leave
     call _int32enable
    ret

global _int32disable
;	void int32disable(void)
_int32disable:
	[bits 32]
	cli
	push eax
	in al, 0x70
	or al, 80h
	out  0x70, al
	pop eax
    ret
global _int32enable
;	void int32enable(void)
_int32enable:
	[bits 32]
	sti
	push eax
	in   al, 0x70
	and  al, 7Fh
	out  0x70, al
	pop eax
    ret