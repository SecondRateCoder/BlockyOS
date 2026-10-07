bits 64
default rel

section .text

global __bochs
__bochs:
	xchg bx, bx
	ret