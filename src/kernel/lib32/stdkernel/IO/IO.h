#include "./kernel/lib32/public/public/public.h"


void iowait(void);

// IO.asm prototypes
extern void ASMCALL outb(uint16_t port, uint8_t value);
extern uint8_t ASMCALL inb(uint16_t port);

extern void ASMCALL outw(uint16_t port, uint16_t value);
extern uint16_t ASMCALL inw(uint16_t port);

extern void ASMCALL outl(uint16_t port, uint32_t value);
extern uint32_t ASMCALL inl(uint16_t port);