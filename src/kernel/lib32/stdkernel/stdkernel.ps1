
$INTERRUPTASM = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/IDT/InterruptRoutines.asm"
$INTERRUPTH = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/IDT/InterruptRoutines.h"
$ERRORINTERRUPTS = @(8, 10, 11, 12, 13, 14, 17, 21, 29, 30)


$HEADERH = 
"#include `"interrupt.h`"
"
$HEADERASM = 
"bits 32
;		**AUTO-GENERATED FILE**

extern isr_handlerC

%macro INTNOERROR 1
global ISR_INTERRUPT%1
ISR_INTERRUPT%1:
    push dword 0	; Push dummy error
    push dword %1	; Push Interrupt Code
	push esp
    jmp isr_inthandle
%endmacro

%macro INTERROR 1
global ISR_INTERRUPT%1
ISR_INTERRUPT%1:
    push dword %1	; Push Interrupt Code
	push esp
    jmp isr_inthandle
%endmacro

extern isr_inthandle

global _interruptTableEnd
global _interruptTableLength
_interruptTableLength:	dd (_interruptTableEnd - _interruptTable)
global _interruptTable
_interruptTable:
"
if(Test-Path $INTERRUPTASM){Remove-Item -Path $INTERRUPTASM -Force}
Add-Content -Path $INTERRUPTASM -Value $HEADERASM

if(Test-Path $INTERRUPTH){Remove-Item -Path $INTERRUPTH -Force}
Add-Content -Path $INTERRUPTH -Value $HEADERH

for ($i = 0; $i -lt 256; $i++){
	if($i -in $ERRORINTERRUPTS){
		Add-Content -Path $INTERRUPTASM -Value "INTERROR $($i)"
	}else{Add-Content -Path $INTERRUPTASM -Value "INTNOERROR $($i)"}
	Add-Content -Path $INTERRUPTH -Value "void __attribute__((cdecl)) ISR_INTERRUPT$($i)(void);"
}

Add-Content -Path $INTERRUPTASM -Value "_interruptTableEnd:	db 0"
