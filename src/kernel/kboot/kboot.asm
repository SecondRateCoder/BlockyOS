bits 64

extern main_km
global kboot_f
;	rax kboot_f(rdi(bootin), rsi(This), rdx(N))
kboot_f:
	;	We need to Initialise the Pointer etc.
	lea rax, qword [rdi]
	mov rsp, rax
	add esp, dword [rdi + 8]
	jmp main_km