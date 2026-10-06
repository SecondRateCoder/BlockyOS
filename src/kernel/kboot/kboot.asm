bits 64
DEFAULT REL

extern __main

section .text

global __bochs_breakpoint
__bochs_breakpoint:
	xchg bx, bx
	ret

;	rax kboot_f(rdi(bootin))
;				bootin = {STACKADDR, STACKSIZE, ...}
global kboot_km
; kboot_km:
; 	;	We need to Initialise the Pointer etc.
; 	call __bochs_breakpoint
; 	lea rax, qword [rdi]
; 	sub dword [rdi + 8], 0x10
; 	add eax, dword [rdi + 8]
; 	mov rsp, rax
; 	jmp __main
kboot_km:
    call __bochs_breakpoint
    
    mov rax, qword [rdi]
    mov rsi, qword [rdi + 8]
    add rax, rsi
    mov rsp, rax

    jmp __main
.loop:
	nop
	jmp .loop