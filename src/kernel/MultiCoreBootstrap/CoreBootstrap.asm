global RealmodeBootstrap

extern LoadBase, BootstrapDummyGDT, BootstrapDummyIDT, CPUBootstrap
section .text

global RealmodeBootstrap
[bits 16]
RealmodeBootstrap:
	;	Disable Interrupts
	cli
	in al, 0x70
	or al, 0x80
	out 0x70, al

	;	Clear the segment registers
	mov ax, 0x10
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	;	Load GDTR
	mov eax, [rel LoadBase]
	add eax, _DTR32
	mov word [eax], 65535
	mov ebx, eax
	add ebx, BootstrapDummyGDT
	mov dword [eax + 2], ebx
	lgdt [eax]

	;	Load IDTR
	mov eax, [rel LoadBase]
	add eax, _DTR32
	mov word [eax], 0
	mov ebx, eax
	add ebx, 0
	mov dword [eax + 2], ebx
	lidt [eax]

	;	Load LDTR
	mov eax, [rel LoadBase]
	add eax, _DTR32
	mov word [eax], 0
	mov ebx, eax
	add ebx, 0
	mov dword [eax + 2], ebx
	lldt [eax]

	mov eax, [rel LoadBase]
	add eax, .ProtectedModeBootstrap
	push word 0x08
	push eax
	retf
[bits 32]
.ProtectedModeBootstrap:
	;	Enable PAE
	mov edx, cr4
	or edx, (1 << 5)

	mov eax, [rel LoadBase]
	add eax, CPUBootstrap
	add eax, 82
	cmp [eax], 0x01
	jne .NoLA57
	or edx, (1 << 12)
.NoLA57:
	mov cr4, edx

	;	Set LME (long mode enable)
	mov ecx, 0xC0000080
	rdmsr
	or eax, (1 << 8)
	wrmsr

	mov eax, [rel LoadBase]
	add eax, CPUBootstrap
	add eax, 0x24
	mov cr3, eax

	;	Enable paging (and protected mode, if it isn't already active)
	or ebx, (1 << 31) | (1 << 0)
	mov cr0, ebx

	;	Now reload the segment registers (CS, DS, SS, etc.) with the appropriate segment selectors...
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
  
	;	Reload CS with a 64-bit code selector by performing a long jmp
	mov eax, [rel LoadBase]
	add eax, .LongModeBootstrap
	push word 0x08
	push eax
	retf
[bits 64]
.LongModeBootstrap:
	;	Initialise the CPU State
	mov rax, [rel LoadBase]
	add rax, CPUBootstrap

	;	Load IDTR (located at offset 72)[cite: 8]
	mov rbx, rax
	add rbx, 72
	lidt [rbx]

	;	Load LDTR (located at offset 62)[cite: 8]
	mov rbx, rax
	add rbx, 62
	lldt [rbx]

	;	Load GDTR (located at offset 52)[cite: 8]
	mov rbx, rax
	add rbx, 52
	lgdt [rbx]

	;	Load CR4 (located at offset 44)[cite: 8]
	mov rbx, [rax + 44]
	mov cr4, rbx

	;	Load CR3 (located at offset 36)[cite: 8]
	mov rbx, [rax + 36]
	mov cr3, rbx

	;	Load CR0 (located at offset 28)[cite: 8]
	mov rbx, [rax + 28]
	mov cr0, rbx

	;	Load RFLAGS (located at offset 20)[cite: 8]
	mov rbx, qword [rax + 20]
	push rbx
	popfq

	;	Load Segment Registers (DS, SS, ES, FS, GS) starting at offset 10[cite: 8]
	mov gs, word [rax + 18]
	mov fs, word [rax + 16]
	mov es, word [rax + 14]
	mov ss, word [rax + 12]
	mov ds, word [rax + 10]

	sub rax, 2
	mov cx, word [rax]
	push cx
	mov rbx, [rel LoadBase]
	add rbx, .LongModeCSRefresh
	push rbx
	retf
.LongModeCSRefresh:
	mov rax, [rel LoadBase]
	add rax, CPUBootstrap
	jmp [rax]

section .bss
_DTR32:	resw	1
		resd	1