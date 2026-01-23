#include "./kernel/public/public/memory/memory.h"
#include "./kernel/public/public/memory/string.h"
#include "./kernel/public/public/math/int/_int.h"


#define PF_DSTEP_LONG 		1
#define PF_DSTEP_LONG_LONG 	4

char g_hexes32[];
#define printf printf32
void printf32(char *fmt, ...);
#define printarg
char *printarg32(uinl32_t *argp, uint8_t dwords, bool sign, uint8_t radix, bool printin, bool attach_sign);

// Function Prototypes
void updateCursor32();
void puts32(char *str);
void putc32(char c, uint8_t color);

// Wrapper prototypes
static inline getCursor32(uinl32_t *X, uinl32_t *Y);
static inline void scrollCursor32(uint8_t lines);

// VGA prototypes
extern void __attribute__((cdecl)) asm_enableCursor32(void);
extern void __attribute__((cdecl)) asm_disdableCursor32(void);
extern uint16_t __attribute__((cdecl)) asm_getCursor32();
extern void __attribute__((cdecl)) asm_updateCursor32(uinl32_t x, uinl32_t y);

// IO.asm prototypes
extern void __attribute__((cdecl)) _outb(uint16_t port, uint8_t value);
extern uint8_t __attribute__((cdecl)) _inb(uint16_t port);
