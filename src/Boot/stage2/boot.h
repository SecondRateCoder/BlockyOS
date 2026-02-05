#pragma once


#include "./kernel/lib32/generic/standard.h"
#include "./kernel/lib32/stdio/stdio.h"
#include "./kernel/lib32/stdkernel/stdkernel.h"
#include "./kernel/lib32/stdmem/32bit/stdmem.h"
#include "localfile/f-rat/f-rat.h"
#include "./kernel/lib32/stdmath/int/_int.h"

#define LOCALSTANDARDFILE
#include "./kernel/lib32/stdprogram/stdprogram.h"

extern uint8_t __TRUECODEADDR, __CODEADDR, __CODEEND, __DATAADDR, __DATAEND, __kernel_end, __kernel_start;

// Halt and restart
extern void ASMCALL start(void);
extern void ASMCALL halt32(void);
extern void ASMCALL bochs_breakpoint32(void);
extern void ASMCALL asm_updateCursor32(uint32_t x, uint32_t y);

// C Types
char g_hexes32[];
extern driveHeader bootDrive;
// C Functions
void setup32();

void updateCursor();
void printf32(char *str, ...);
static inline void scrollCursor32(uint8_t lines);
char *printarg32(uint32_t *argp, uint8_t doubles, bool sign, uint8_t radix, bool printin, bool attach_sign);


extern void switch16_32(void *gdt, void *idt, void( ASMCALL *func)(void));

typedef struct BootIn{
    MemRegion Memory;
}BootIn;