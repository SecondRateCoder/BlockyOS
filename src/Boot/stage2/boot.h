#pragma once

#include "./kernel/lib32/public/kernpublic.h"

// Halt and restart
extern void ASMCALL start(void);
extern void ASMCALL halt(void);
extern void ASMCALL bochs_breakpoint(void);
extern void ASMCALL asm_updateCursor32(uinl32_t x, uinl32_t y);
// Division helpers
#define div64_32(dividend, divisor, result, remainder) div64_32_((uint64_t)(dividend), (uinl32_t)(divisor), (uint64_t  *)(result), (uinl32_t  *)(remainder))
extern void ASMCALL div64_32_(uint64_t dividend, uinl32_t divisor, uint64_t  *result, uinl32_t  *remainder);

// C Types
char g_hexes32[];
extern drive_header bt1_drive_header;
// C Functions
void setup32();

void updateCursor();
void printf32(char *str, ...);
static inline void scrollCursor32(uint8_t lines);
char *printarg32(uinl32_t *argp, uint8_t doubles, bool sign, uint8_t radix, bool printin, bool attach_sign);


extern void switch16_32(void *gdt, void *idt, void( ASMCALL *func)(void));
