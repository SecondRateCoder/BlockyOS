bits 32

extern VGA_MAXX
extern VGA_MAXY


_asm_TestVGA:						; Test that VGA is active, otherwise emulate.
	; Implement at sometime
	ret

global _asm_disableCursor32
;	void asm_disableCursor32(void)
_asm_disableCursor32:
	outb 0x3D4, 0x0A
	outb 0x3D5, 0x20
	ret

global _asm_getCursor32:
;	uint16_t asm_getCursor32(void)
_asm_getCursor32:
	push eax
	mov eax, dword 0
	outb 0x3D4, 0x0F
	inb 0x3D5, al

	outb 0x3D4, 0x0E
	inb 0x3D5, ah
	ret
	

global _asm_enableCursor32
;	void asm_enableCursor32(uinl32_t, uinl32_t)
_asm_enableCursor32:
	; Save Regs
	push eax
	outb 0x3D4, 0x0A				; Setup for Cursor End
	inb 0x3D5, al
	and eax, 0xC0
	or eax, [ebp + 12]
	outb 0x3D5, al					; Write Cursor End

	outb 0x3D4, 0x0B				; Setup for Cursor Start
	inb 0x3D5, al
	and eax, 0xE0
	or eax, [ebp + 16]
	outb 0x3D5, al					; Write Cursor End
	; Restore Regs
    pop eax
    ret

global _asm_updateCursor32
;	void asm_UpdateCursor32(uinl32_t, uinl32_t)
_asm_updateCursor32:
	; Save Regs
	push eax
	mov eax, [ebp + 16]				; Load Y
	mul word [VGA_MAXX]				; Multiply by MAX rows
	add eax, [ebp + 12]				; Add X
	shr eax, 8						; Get high byte
	outb 0x3D4, 0x0F				; Setup for writing low byte
	outb 0x3D5, al					; Write low byte
	outb 0x3D4, 0x0E				; Setup for writing High byte
	outb 0x3D5, al					; Write high byte
	; Restore Regs
    pop eax
    ret
