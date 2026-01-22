bits 32

global _asm_updateCursor32
;	void asm_UpdateCursor32(uinl32_t, uinl32_t)
_asm_updateCursor32:
	push bp
	mov bp, sp
	push eax

    pop eax
    leave
    ret