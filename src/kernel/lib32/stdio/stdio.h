#pragma once

#include "./kernel/lib32/generic/standard.h"

#define ANSI_COLOR(R, G, B)
#define ENDL "\n\r\0"

uint32_t VGAX, VGAY;

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
char *printarg32(uint32_t *argp, uint8_t dwords, bool sign, uint8_t radix, bool printin, bool attach_sign);


// Function Prototypes
void updateCursor32();
void puts32(char *str);
void putc32(char c);
static inline void setColor(uint8_t color);

// Wrapper prototypes
static inline void getCursor32(uint32_t *X, uint32_t *Y);
static inline void scrollCursor32(uint8_t lines);

// VGA prototypes
extern void ASMCALL asm_enableCursor32(void);
extern void ASMCALL asm_disdableCursor32(void);
extern uint16_t ASMCALL asm_getCursor32();
extern void ASMCALL asm_updateCursor32(uint32_t x, uint32_t y);
