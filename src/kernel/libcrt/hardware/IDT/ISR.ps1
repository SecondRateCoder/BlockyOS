
$OUT = Join-Path (Get-Location) 'src\kernel\libcrt\hardware\IDT\ISR.asm'

$MAX = 256
$ERRS = @(8, 10, 11, 12, 13, 14, 17, 21)
$PREFIX = 'bits 64
extern InterruptCallbacks

; For interrupts w/o a CPU error code
%macro isrCallbackNoError 1
global isrCallback%1
isrCallback%1:
    push qword 0    ; Dummy error code (for uniform stack frame layout)
    push qword %1   ; Vector number
    jmp common_isr_stub
%endmacro

; For interrupts w a CPU error code (CPU already pushed error code)
%macro isrCallbackWError 1
global isrCallback%1
isrCallback%1:
    push qword %1   ; Vector number
    jmp common_isr_stub
%endmacro

section .text

common_isr_stub:
    ; Save all general-purpose registers
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

    ; Save vector number before aligning stack
    mov rbx, [rsp + 120]    ; Vector number is above 15 registers (15 * 8 = 120)

    ; Pass InterruptStackFrame pointer as 1st parameter (RDI) in System V ABI
    mov rdi, rsp

    ; Align RSP to 16 bytes for System V ABI compliance
    mov rbp, rsp            ; Save unaligned RSP in RBP (RBP is preserved across C calls)
    and rsp, -16            ; Align RSP to 16-byte boundary

    cld                     ; Clear direction flag for standard C ABI

    ; Call C handler from callback array using the saved vector number
    call qword [InterruptCallbacks + (8 * rbx)]

    ; Restore unaligned stack frame pointer
    mov rsp, rbp

    ; Restore registers in reverse order
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

    ; Clean up Vector Number and Error Code (or Dummy Error Code)
    add rsp, 16

    ; Return from interrupt
    iretq'

Set-Content -Path $OUT -Value $PREFIX

for($i = 0; $i -lt $MAX; $i++){
    if($ERRS -contains $i){
        Add-Content -Path $OUT -Value "isrCallbackWError $($i)"
    }else{Add-Content -Path $OUT -Value "isrCallbackNoError $($i)"}
}