bits 32

extern VGA_MAXX, VGA_MAXY


asm_TestVGA32:						; Test that VGA is active, otherwise emulate.
	; Implement at sometime
	ret

global asm_disableCursor32
;	void asm_disableCursor32(void)
asm_disableCursor32:
	mov dx, 0x3D4
	mov al, 0x0A
	out dx, al
	inc dx
	mov al, 0x20
	out dx, al
	ret

global asm_getCursor32:
;	uint16_t asm_getCursor32(void)
asm_getCursor32:
	mov dx, 0x3D4
	mov al, 0x0F
	out dx, al

	inc dx
	in al, dx

	in al, dx
	shl ax, 8

	dec dx
	mov al, 0x0E
	out dx, al
	ret
	

global asm_enableCursor32
;	void asm_enableCursor32(uint32_t, uint32_t)
asm_enableCursor32:
	mov dx, 0x3D4
	mov al, 0x0A
	out dx, al				; Setup for Cursor End

	inc dx
	in al, dx
	and eax, 0xC0
	or eax, [ebp + 12]
	out dx, al					; Write Cursor End

	dec dx
	mov al, 0x0B
	out dx, al				; Setup for Cursor Start

	inc dx
	in al, dx
	and eax, 0xE0
	or eax, [ebp + 16]
	out dx, al					; Write Cursor End

    ret

global asm_updateCursor32
;	void asm_UpdateCursor32(uint32_t, uint32_t)
asm_updateCursor32:
	mov eax, [ebp + 16]				; Load Y
	mul word [VGA_MAXX]				; Multiply by MAX rows
	add eax, [ebp + 12]				; Add X
	shr eax, 8						; Get high byte

	mov dx, 0x3D4
	mov al, 0x0F
	out dx, al				; Setup for writing low byte

	inc dx
	out dx, al					; Write low byte

	dec dx
	mov al , 0x0E
	out dx, al				; Setup for writing High byte

	inc dx
	out dx, al					; Write high byte

    ret
