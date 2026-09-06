bits 64
section .text

%define CR4Level5PagingBit		(1 << 12)
%define ErrorExpectsLevel5Code	0
%define ErrorExpectsLevel4Code	2

%define true 1
%define false 0

global IsLA57Enabled
IsLA57Enabled:
	mov rax, cr4
	shr rax, 12
	and rax, true
	ret

global EnableLA57
EnableLA57:
	mov rax, cr4
	or rax, CR4Level5PagingBit
	mov cr4, rax
	mov rax, 0x0
	ret

global IsLA57Supported
IsLA57Supported:
	mov rax, 7
	cpuid
	mov eax, ecx
	shr eax, 16
	; nand eax, true
	and eax, true
	ret

global LoadPageTree
;	typedef void PageTreeFunction()
;	eax(return) LoadPageTree(rdi(PageTree *), rsi(Function Pointer), rdx(Data Segment), rcx(Code Segment), r8(Level5 Table))
LoadPageTree:
	;	Check for whether we need to load Level5 Paging
	call IsLA57Supported
	cmp rax, r8
	je .Load
	;	If 
	jg .ErrorExpectsLevel5
	;	jl .ErrorExpectsLevel4
	mov eax, ErrorExpectsLevel4Code
	ret
.ErrorExpectsLevel5:
	mov eax, ErrorExpectsLevel5Code
	ret
.Load:
	mov rax, rdi
	mov cr3, rax

	mov rax, rdx
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	jmp rsi

extern MapPTVirtual
global GetPageTable
;	rax(Pointer)GetPageTable(rdi(bool *))
GetPageTable:
	call IsLA57Enabled
	cmp rax, true
	jne .Level5Disabled
	mov [rdi], byte true
.Level5Disabled:
	mov rax, cr3
	mov rdi, rax
	call MapPTVirtual
	ret
