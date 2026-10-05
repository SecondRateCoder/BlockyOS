bits 64

extern __main
global kboot_km

section .text
;	rax kboot_f(rdi(bootin))
kboot_km:
	;	We need to Initialise the Pointer etc.
	lea rax, qword [rdi]
	add eax, dword [rdi + 8]
	mov rsp, rax
	jmp __main