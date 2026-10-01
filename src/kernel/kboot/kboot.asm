bits 64

extern __main
global kboot_km

section .text
;	rax kboot_f(rdi(bootin), rsi(This), rdx(N))
kboot_km:
	;	We need to Initialise the Pointer etc.
	lea rax, qword [rdi]
	mov rsp, rax
	add esp, dword [rdi + 8]
	jmp __main