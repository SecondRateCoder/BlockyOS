bits 64
default rel

global __bochs_breakpoint
__bochs_breakpoint:
	xchg bx, bx
	ret

global __e9print, __e9printc
__e9printc:
	mov al, dil
	out 0xE9, al
	mov eax, 1
	ret
__e9print:
.loop:
	mov al, byte [rdi]
	out 0xE9, al
	dec rsi
	add rdi, 2
	cmp rsi, 0
	je .finish
	jmp .loop
.finish:
	mov eax, 1
	ret