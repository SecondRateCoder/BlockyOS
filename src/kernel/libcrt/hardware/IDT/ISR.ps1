$OUT = Join-Path (Get-Location) 'src\kernel\libcrt\hardware\IDT\ISR.asm'
$HDR = Join-Path (Get-Location) 'src\kernel\libcrt\hardware\IDT\ISRCallback.h'

$HDRPrefix = '#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
'

$MAX = 256
$ERRS = @(8, 10, 11, 12, 13, 14, 17, 21, 29, 30)
$PREFIX = 'bits 64
extern InterruptCallbacks
extern __bochs

;	For interrupts w/o a CPU error code
%macro isrCallbackNoError 1
global isrCallback%1
isrCallback%1:
	push qword 0    ; Dummy error code (for uniform stack frame layout)
	push qword %1   ; Vector number
	jmp common_isr_stub
%endmacro

;	For interrupts w/ a CPU error code (CPU already pushed error code)
%macro isrCallbackWError 1
global isrCallback%1
isrCallback%1:
	push qword %1   ; Vector number
	jmp common_isr_stub
%endmacro

section .text
global common_isr_stub
common_isr_stub:
	;	Save all general-purpose registers
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov rax, cr2
	push rax

	;	Save vector number before aligning stack
	mov rbx, [rsp + 120]    ; Vector number is above 15 registers (15 * 8 = 120)

	;	Pass InterruptStackFrame pointer as 1st parameter (RDI) in System V ABI
	mov rdi, rsp

	;	Align RSP to 16 bytes for System V ABI compliance
	mov rbp, rsp            ; Save unaligned RSP in RBP (RBP is preserved across C calls)
	and rsp, -16            ; Align RSP to 16-byte boundary

	cld                     ; Clear direction flag for standard C ABI

	;	Assuming each callback entry is 16 bytes and the function pointer is at offset 0:
	lea rsi, [rel InterruptCallbacks]
	mov rax, [rsi + (rbx * 8)]	;	Load the function pointer directly using base + index * scale
	call rax					;	Call the function pointer

	;	Restore unaligned stack frame pointer
	mov rsp, rbp

	;	Pop the CR2 Register.
	pop r15
	;	Restore registers in reverse order
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax

	;	Clean up Vector Number and Error Code (or Dummy Error Code)
	add rsp, 16

	;	Return from interrupt
	iretq

'

Set-Content -Path $OUT -Value $PREFIX
Set-Content -Path $HDR -Value $HDRPrefix

for($i = 0; $i -lt $MAX; $i++){
	Add-Content -Path $HDR -Value "extern __naked void isrCallback$($i)(void);"
	if($ERRS -contains $i){
		Add-Content -Path $OUT -Value "isrCallbackWError $($i)"
	}else{Add-Content -Path $OUT -Value "isrCallbackNoError $($i)"}
}

Add-Content -Path $HDR -Value "#define IDTCallbackInit(IDT)`t\"
for($i = 0; $i -lt $MAX; $i++){
	#	Every line of the macro continues onto the next with a backslash, EXCEPT the very last one: a backslash at the
	#	end of the file is a warning ("backslash-newline at end of file") and silently joins whatever follows.
	$CONT = if($i -lt ($MAX - 1)){"`t`t`t\"}else{""}
	Add-Content -Path $HDR -Value "`t(IDT)[$($i)].OffsetHigh = ((uint64_t)isrCallback$($i) >> 32) & UINT32_MAX;`t\"
	Add-Content -Path $HDR -Value "`t(IDT)[$($i)].OffsetMid = ((uint64_t)isrCallback$($i) >> 16) & UINT16_MAX;`t\"
	Add-Content -Path $HDR -Value "`t(IDT)[$($i)].OffsetLow = (uint64_t)isrCallback$($i) & UINT16_MAX;$($CONT)"
}