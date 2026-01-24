#pragma once

#include "./kernel/public/public/public.h"
#include "./kernel/lib32/stdfile/stdfile.h"

// Halt and restart
extern void __attribute__((cdecl)) start(void);
extern void __attribute__((cdecl)) halt(void);
extern void __attribute__((cdecl)) bochs_breakpoint(void);
extern void __attribute__((cdecl)) asm_updateCursor32(uinl32_t x, uinl32_t y);
// Division helpers
#define div64_32(dividend, divisor, result, remainder) div64_32_((uint64_t)(dividend), (uinl32_t)(divisor), (uint64_t  *)(result), (uinl32_t  *)(remainder))
extern void __attribute__((cdecl)) div64_32_(uint64_t dividend, uinl32_t divisor, uint64_t  *result, uinl32_t  *remainder);
extern unsigned short __attribute__((cdecl)) __U8LS(unsigned char dividend, unsigned char divisor);
extern short __attribute__((cdecl)) __I8LS(signed char value, unsigned char shift);

// C Types
char g_hexes32[];
extern drive_header bt1_drive_header;
// C Functions
void main32(uint16_t bootDrive);

void updateCursor();
void printf32(char *str, ...);
static inline void scrollCursor32(uint8_t lines);
char *printarg32(uinl32_t *argp, uint8_t doubles, bool sign, uint8_t radix, bool printin, bool attach_sign);

#define ENDL "\r\n\0"

extern void int32disable(void);
extern void int32enable(void);
extern void switch16_32(void *gdt, void *idt, void( __attribute__((cdecl)) *func)(void));
