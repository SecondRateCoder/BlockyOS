#pragma once


#include "./kernel/lib32/generic/standard.h"
#include "./kernel/lib32/stdio/stdio.h"
#include "./kernel/lib32/stdkernel/stdkernel.h"
// #include "./kernel/lib32/stdkernel/Interrupt/interrupt.h"
#include "./kernel/lib32/stdmem/32/stdmem.h"
#include "localfile/f-rat/f-rat.h"
#include "./kernel/lib32/stdmath/int/_int.h"
#include "./kernel/lib32/stdprogram/stdprogram.h"

extern idtDESC_t FORCE_SYMBOLEXPOSURE IDTdesc;
extern gdtDESC_t FORCE_SYMBOLEXPOSURE GDTdesc;

extern const uint16_t GDTSize, GDTSize;
extern gdtENTRY_t GDT[];
extern idtENTRY_t IDT[];


extern uint8_t __TRUECODEADDR, __CODEADDR, __CODEEND, __DATAADDR, __DATAEND, __kernel_end, __kernel_start;

// Halt and restart
extern void ASMCALL start(void);
extern void ASMCALL halt32(void);
extern void ASMCALL bochs_breakpoint32(void);
extern void ASMCALL asm_updateCursor32(uint32_t x, uint32_t y);

// C Types
extern driveHeader bootDrive;
// C Functions
void setup32();

typedef struct BootIn{
    MemRegion Memory;
}BootIn;
