bits 64
default rel

extern __bochs

%define SIMDReqSize	512
%define true		1
%define false		0
%define AVX			64
%define AVX2		128

section .text

global SIMDSupported
SIMDSupported:
	mov eax, 0x1
	cpuid
	test edx, (1 << 25)
	jz .noSupport
	mov eax, true
	ret
.noSupport:
	mov eax, false
	ret

global CheckSIMDExtensions
CheckSIMDExtensions:
	mov eax, 0x1
	cpuid
	xor eax, eax
	test edx, (1 << 25)
	jz .check_sse2
	or eax, 1                ; SSE
.check_sse2:
	test edx, (1 << 26)
	jz .check_sse3
	or eax, 2                ; SSE2
.check_sse3:
	test ecx, (1 << 0)
	jz .check_ssse3
	or eax, 4                ; SSE3
.check_ssse3:
	test ecx, (1 << 9)
	jz .check_sse4_1
	or eax, 8                ; SSSE3
.check_sse4_1:
	test ecx, (1 << 19)
	jz .check_sse4_2
	or eax, 16               ; SSE4.1
.check_sse4_2:
	test ecx, (1 << 20)
	jz .check_avx
	or eax, 32               ; SSE4.2
.check_avx:
	test ecx, (1 << 27)
	jz .check_avx2           ; OSXSAVE required for AVX
	test ecx, (1 << 28)
	jz .check_avx2           ; CPU supports AVX
	mov ecx, 0
	xgetbv
	and eax, 3              ; XCR0[1:0] must be set for XMM/YMM state
	cmp eax, 3
	jne .check_avx2
	or eax, AVX             ; AVX
.check_avx2:
	mov eax, 0x7
	mov ecx, 0
	cpuid
	test ebx, (1 << 5)
	jz .done                ; AVX2
	and eax, AVX            ; preserve AVX support state
	jz .done
	or eax, AVX2
.done:
	ret

;	RDI = output buffer, RSI = feature mask
;	Returns RAX = output buffer pointer
global DecodeSIMDFeatureMask
DecodeSIMDFeatureMask:
	mov rax, rdi
	test rsi, 1
	jz .skip_sse
	lea rdx, strSSE
	call .copySIMDString
.skip_sse:
	test rsi, 2
	jz .skip_sse2
	lea rdx, strSSE2
	call .copySIMDString
.skip_sse2:
	test rsi, 4
	jz .skip_sse3
	lea rdx, strSSE3
	call .copySIMDString
.skip_sse3:
	test rsi, 8
	jz .skip_ssse3
	lea rdx, strSSSE3
	call .copySIMDString
.skip_ssse3:
	test rsi, 16
	jz .skip_sse4_1
	lea rdx, strSSE4_1
	call .copySIMDString
.skip_sse4_1:
	test rsi, 32
	jz .skip_sse4_2
	lea rdx, strSSE4_2
	call .copySIMDString
.skip_sse4_2:
	test rsi, AVX
	jz .skip_avx
	lea rdx, strAVX
	call .copySIMDString
.skip_avx:
	test rsi, AVX2
	jz .done_decode
	lea rdx, strAVX2
	call .copySIMDString
.done_decode:
	cmp rdi, rax
	jne .trim_result
	lea rdx, strNONE
	call .copySIMDStringNoSpace
	jmp .return
.trim_result:
	dec rdi
	mov byte [rdi], 0
.return:
	mov rax, rax
	ret
.copySIMDString:
	mov rsi, rdx
.copySIMDStringLoop:
	mov al, [rsi]
	mov [rdi], al
	inc rsi
	inc rdi
	test al, al
	jnz .copySIMDStringLoop
	dec rdi
	mov byte [rdi], ' '
	inc rdi
	mov byte [rdi], 0
	ret
.copySIMDStringNoSpace:
	mov rsi, rdx
.copySIMDStringNoSpaceLoop:
	mov al, [rsi]
	mov [rdi], al
	inc rsi
	inc rdi
	test al, al
	jnz .copySIMDStringNoSpaceLoop
	ret

global EnableSIMD
EnableSIMD:
	call SIMDSupported
	cmp rax, false
	je .SSEDisabled
	mov rax, cr0
	and ax, 0xFFFB		; clear coprocessor emulation CR0.EM
	or ax, 0x2			; set coprocessor monitoring CR0.MP
	mov cr0, rax
	mov rax, cr4
	or ax, (3 << 9)		; set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	mov cr4, rax
	mov eax, 1
	ret
.SSEDisabled:
	mov eax, 0
	ret
global DisableSIMD
DisableSIMD:
	mov rax, cr4
	and rax, ~(3 << 9)     ; clear CR4.OSFXSR and CR4.OSXMMEXCPT
	mov cr4, rax
	mov rax, cr0
	or eax, 0x4            ; set CR0.EM to disable floating point/SSE instructions
	mov cr0, rax
	mov rax, 1
	ret

;	Align the Inputted Memory to the 16-bytes
global StoreSSEState
StoreSSEState:
	; call __bochs
	call EnableSIMD
	cmp eax, true
	je .checkRoundedMemory
	lea rax, StoreSSEStateNotEnabled
	ret
.checkRoundedMemory:
	;	Check for rounded Memory
	mov rdx, 0
	mov rax, rdi
	mov rcx, 16
	div rcx
	cmp rdx, 0
	je .alignedMemory
	lea rax, StoreSSEStateAlignedMemoryError
	ret
.alignedMemory:
	cmp rsi, SIMDReqSize
	jge .enoughBytes
	lea rax, StoreSSEStateEnoughBytesError
	ret
.enoughBytes:
	fxsave [rdi]
	mov rax, 0
	ret

;	Align the Inputted Memory to the 16-bytes
global LoadSSEState
LoadSSEState:
	; call __bochs
	call EnableSIMD
	cmp eax, true
	je .checkRoundedMemory
	lea rax, StoreSSEStateNotEnabled
	ret
.checkRoundedMemory:
	;	Check for rounded Memory
	mov rdx, 0
	mov rax, rdi
	mov rcx, 16
	div rcx
	cmp rdx, 0
	je .alignedMemory
	lea rax, StoreSSEStateAlignedMemoryError
	ret
.alignedMemory:
	cmp rsi, SIMDReqSize
	je .enoughBytes
	lea rax, StoreSSEStateEnoughBytesError
	ret
.enoughBytes:
	fxrstor [rdi]
	mov rax, 0
	ret

segment .data
strSSE:						db 'SSE', 0
strSSE2:					db 'SSE2', 0
strSSE3:					db 'SSE3', 0
strSSSE3:					db 'SSSE3', 0
strSSE4_1:					db 'SSE4.1', 0
strSSE4_2:					db 'SSE4.2', 0
strAVX:						db 'AVX', 0
strAVX2:					db 'AVX2', 0
strNONE:					db 'NONE', 0
StoreSSEStateAlignedMemoryError:db 'Input Memory was not aligned to the 16-byte Boundary', 0
StoreSSEStateEnoughBytesError:	db 'Input Memory was not of the Required Size', 0
StoreSSEStateNotEnabled:		db 'SSE is not Enabled', 0