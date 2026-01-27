
$INTERRUTASM = Join-Path (Get-Location) "src/kernel/lib32/stdlib/InterruptRoutines.asm"

$ERRORINTERRUPTS = @(8, 10, 11, 12, 13, 14, 17, 21, 29, 30)

$HEADER = "
bits 32
;		**AUTO-GENERATED FILE**

extern isr_handlerC

%macro INTNOERROR 1
    push dword 0
    push dword %1
	push esp
	pushad
	push esp
    jmp isr_inthandle
%endmacro

%macro INTERROR 1
    push dword %1
	push esp
	pushad
	push esp
    jmp isr_inthandle
%endmacro

isr_inthandle:
    pusha                       ; save general-purpose registers
    push ds
    push es
    mov ax, KERNEL_ISRSEG       ; kernel segment (example)
    mov ds, ax
    mov es, ax
	call isr_handlerC
	pop es
	pop ds
	popa
	iret

global _interruptTableEnd
global _interruptTableLength
_interruptTableLength:	dd (_interruptTableEnd - _interruptTable)
global _interruptTable
_interruptTable:
"
if(Test-Path $INTERRUTASM){Remove-Item -Path $INTERRUTASM -Force}
Add-Content -Path $INTERRUTASM -Value $HEADER

for ($i = 0; $i -lt 255; $i++){
	if($i -in $ERRORINTERRUPTS){
		Add-Content -Path $INTERRUTASM -Value "INTERROR $($i)"
	}else{Add-Content -Path $INTERRUTASM -Value "INTNOERROR $($i)"}
}

Add-Content -Path $INTERRUPTASM -Value "_interruptTableEnd:	db 0
