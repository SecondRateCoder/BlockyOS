bits 64
default rel

extern __main
extern __bochs

section .text
;	rax kboot_f(rdi(bootin))
;				bootin = {STACKADDR, STACKSIZE, ...}
global kboot_km
kboot_km:
    call __bochs
    
    mov rax, qword [rdi]
    add eax, dword [rdi + 8]
    mov rsp, rax

    jmp __main
.loop:
	nop
	jmp .loop