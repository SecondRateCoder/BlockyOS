
$INTERRUPTASM = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/Interrupt/InterruptRoutines.asm"
$INTERRUPTH = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/Interrupt/InterruptRoutines.h"
$INTERRUPTC = Join-Path (Get-Location) "src/kernel/lib32/stdkernel/Interrupt/InterruptRoutines.c"
$ERRORINTERRUPTS = @(8, 10, 11, 12, 13, 14, 17, 21, 29, 30)
$EXCLUDE = @()

function Write-Atomic{
    param(
        [string]$Path,
        [string[]]$Lines
    )

    $tmp = "$($Path).tmp"

    # Write full content to temp
    Set-Content -Path $tmp -Value $Lines -Encoding Default

    # Atomically replace
    Move-Item -Path $tmp -Destination $Path -Force
}

$InformationPreference = 'SilentlyContinue'
$WarningPreference = 'SilentlyContinue'
$ProgressPreference = 'SilentlyContinue'

$asmLines = @()
$asmLines += "bits 32
;       **AUTO-GENERATED SCRIPT**
extern isr_inthandle

%macro INTNOERROR 1
global ISR_INTERRUPT%1
ISR_INTERRUPT%1:
    push dword 0    ; Push dummy error
    push dword %1   ; Push Interrupt Code
    push esp
    jmp isr_inthandle
%endmacro

%macro INTERROR 1
global ISR_INTERRUPT%1
ISR_INTERRUPT%1:
    push dword %1   ; Push Interrupt Code
    push esp
    jmp isr_inthandle
%endmacro


global _interruptTableEnd
global _interruptTableLength
_interruptTableLength:  dd (_interruptTableEnd - _interruptTable)
global _interruptTable
_interruptTable:
".TrimEnd()
for($i = 0; $i -lt 256; $i++){
    if($i -notin $EXCLUDE){
        if($i -in $ERRORINTERRUPTS){$asmLines += "INTERROR $i"
        }else{$asmLines += "INTNOERROR $i"}
    }
}
$asmLines += "_interruptTableEnd:"

$hLines = @()
$hLines += "#pragma once
//      **AUTO-GENERATED SCRIPT**
#include `"./kernel/lib32/generic/standard.h`"
#include `"interrupt.h`"
".TrimEnd()

for($i = 0; $i -lt 256; $i++){
    if($i -notin $EXCLUDE){$hLines += "extern void ASMCALL ISR_INTERRUPT$($i)();"}
}

$cLines = @()
$cLines += "//      **AUTO-GENERATED SCRIPT**
#include `"InterruptRoutines.h`"

void InitIDT(idtENTRY_t *IDT, uint16_t limit, uint16_t CodeSegment){
    idtDESC_t temp = {.table = IDT, .limit = limit};
    LoadIDT(&temp);
".TrimEnd()

for($i = 0; $i -lt 256; $i++){
    if($i -notin $EXCLUDE){
        $cLines += "`tInitInterrupt($i, true, ISR_INTERRUPT$($i), CodeSegment, 
            IDTFLAGS_RING0 | IDTFLAGS32B_INTRGATE | IDTFLAGS_PRESENT);"
    }
}
$cLines += "}"

Write-Atomic -Path $INTERRUPTASM -Lines $asmLines
Write-Atomic -Path $INTERRUPTC -Lines $cLines
Write-Atomic -Path $INTERRUPTH -Lines $hLines