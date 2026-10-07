bits 64
default rel

global __bochs_breakpoint
__bochs_breakpoint:
	xchg bx, bx
	ret