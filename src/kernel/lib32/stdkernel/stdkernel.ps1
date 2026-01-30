
$INTERRUPTASM = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/Interrupt/InterruptRoutines.asm"
$INTERRUPTH = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/Interrupt/InterruptRoutines.h"
$INTERRUPTC = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/Interrupt/InterruptRoutines.c"
$ERRORINTERRUPTS = @(8, 10, 11, 12, 13, 14, 17, 21, 29, 30)
$EXCLUDE = @()
for($i = 0; $i -lt 16; ++$i){$EXCLUDE += ($i + 32)}
$OK = $true
$ErrorActionPreference = "Ignore"

$HEADERC = 
"//		**AUTO-GENERATED SCRIPT**
#include `"interrupt.h`"

void InitIDT(IDTentry *IDT, uint16_t CodeSegment){
	LoadIDT(IDT);
	IRQInit(IDT);
"

$HEADERH = 
"//		**AUTO-GENERATED SCRIPT**
#include `"interrupt.h`"
"
$HEADERASM = 
"bits 32
;		**AUTO-GENERATED SCRIPT**

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
do{
	$OK = $true
	try{Add-Content -Path $INTERRUPTASM -Value $HEADERASM
	}catch{$OK = $false}
}while($OK -eq $false)

if(Test-Path $INTERRUPTH){Remove-Item -Path $INTERRUPTH -Force}
do{
	$OK = $true
	try{Add-Content -Path $INTERRUPTH -Value $HEADERH
	}catch{$OK = $false}
}while($OK -eq $false)

if(Test-Path $INTERRUPTC){Remove-Item -Path $INTERRUPTC -Force}
do{
	$OK = $true
	try{Add-Content -Path $INTERRUPTC -Value $HEADERC
	}catch{$OK = $false}
}while($OK -eq $false)

for ($i = 0; $i -lt 256; $i++){
	if($i -notin $EXCLUDE){
		if($i -in $ERRORINTERRUPTS){
			do{
				$OK = $true
				try{Add-Content -Path $INTERRUPTASM -Value "INTERROR $($i)"
				}catch{$OK = $false}
			}while($OK -eq $false)
		}else{
			do{
				$OK = $true
				try{Add-Content -Path $INTERRUPTASM -Value "INTNOERROR $($i)"
				}catch{$OK = $false}
			}while($OK -eq $false)
		}
		do{
			$OK = $true
			try{Add-Content -Path $INTERRUPTH -Value "extern void ASMCALL ISR_INTERRUPT$($i)(void);"
			}catch{$OK = $false}
		}while($OK -eq $false)
		do{
			$OK = $true
			try{Add-Content -Path $INTERRUPTC -Value "`tInitInterrupt(
					IDT, $($i), true, ISR_INTERRUPT$($i), CodeSegment, 
					IDTFLAGS_RING0 | IDTFLAGS32B_INTRGATE | IDTFLAGS_PRESENT);"
			}catch{$OK = $false}
		}while($OK -eq $false)
		
	}
}

Add-Content -Path $INTERRUPTASM -Value "_interruptTableEnd:	db 0"

Add-Content -Path $INTERRUPTC -Value "}"
