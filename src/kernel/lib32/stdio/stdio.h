#pragma once

#include "./kernel/public/public/memory/memory.h"
#include "./kernel/public/public/memory/string.h"
#include "./kernel/public/public/math/int/_int.h"

#define ANSI_COLOR(R, G, B)

uinl32_t VGAX, VGAY;

#define ANSI_RED 4
#define ANSI_GREEN 2
#define ANSI_YELLOW 14
#define ANSI_BLUE 1
#define ANSI_LBLUE 9
#define ANSI_LMAGENTA 13
#define ANSI_CYAN 3

#define PF_DSTEP_LONG 		1
#define PF_DSTEP_LONG_LONG 	4

char g_hexes32[];
uint8_t g_color;

#define printf printf32
#define printarg printarg32
#define clear32 memset(VGA, VGA_MAXX * VGA_MAXY, 0)
#define putc putc32
#define puts puts32

void printf32(char *fmt, ...);
char *printarg32(uinl32_t *argp, uint8_t dwords, bool sign, uint8_t radix, bool printin, bool attach_sign);


// Function Prototypes
void updateCursor32();
void puts32(char *str);
void putc32(char c);
static inline void setColor(uint8_t color);

// Wrapper prototypes
static inline void getCursor32(uinl32_t *X, uinl32_t *Y);
static inline void scrollCursor32(uint8_t lines);

// VGA prototypes
extern void __attribute__((cdecl)) asm_enableCursor32(void);
extern void __attribute__((cdecl)) asm_disdableCursor32(void);
extern uint16_t __attribute__((cdecl)) asm_getCursor32();
extern void __attribute__((cdecl)) asm_updateCursor32(uinl32_t x, uinl32_t y);

// IO.asm prototypes
extern void __attribute__((cdecl)) outb(uint16_t port, uint8_t value);
extern uint8_t __attribute__((cdecl)) inb(uint16_t port);
