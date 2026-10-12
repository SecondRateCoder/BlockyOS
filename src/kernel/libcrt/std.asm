bits 64
default rel

section .text

global __bochs
__bochs:
	xchg bx, bx
	ret

global e9print, e9printc
e9printc:
	mov al, dil
	out 0xE9, al
	mov eax, 1
	ret
e9print:
.loop:
	mov al, byte [rdi]
	out 0xE9, al
	dec rsi
	inc rdi
	cmp rsi, 0
	je .finish
	jmp .loop
.finish:
	mov eax, 1
	ret