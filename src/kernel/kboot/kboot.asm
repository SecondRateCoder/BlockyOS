bits 64
default rel

extern __main
; extern __bochs

section .text
;	rax kboot_f(rdi(bootin))
;				bootin = {STACKADDR, STACKSIZE, ...}
global kboot_km
kboot_km:
	cli
    ; call __bochs
    mov rax, 0
    mov eax, dword [rdi + 8]
    add rax, qword [rdi]
	sub eax, 0x0200
    mov rsp, rax

    jmp __main
.loop:
	nop
	jmp .loop