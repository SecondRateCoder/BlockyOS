bits 32

extern isr_handlerC

isr_inthandle:
    pusha                       ; save general-purpose registers
    xor eax, eax
    mov ax, ds
    push eax

    mov ax, 0x10               ; kernel data segment

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
	iret

global _LoadIDT
;   void LoadIDT(idtDESC_t *)
_LoadIDT:
    push bp
    mov bp, sp
    mov eax, [bp + 8]   ; Not sure if correct
    lidt [eax]
    leave
    ret