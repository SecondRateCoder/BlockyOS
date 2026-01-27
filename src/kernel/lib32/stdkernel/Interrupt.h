#pragma once

#include "./kernel/public/public/math/math.h"

// Should be at address NULL, 0x00
IDTentry IDTtable[256] = 0x00;

typedef void __attribute__((interrupt)) (*interruptEntry)(void);
extern interruptEntry interruptTable[256];
extern __attribute__((packed)) uint8_t interruptTableEnd;
extern uint32_t interruptTableLength;

typedef struct IDTentry{
    uint16_t baseLow;
    uint16_t segment;
    uint8_t reserved;
    uint8_t flags;
    uint16_t baseHigh;
}__attribute__((packed)) IDTentry;

typedef struct IDTDesc{
    uint16_t size;
    uint32_t table;
}__attribute__((packed)) IDTDesc;

typedef enum IDTFLAGS{
    IDTFLAGS_TSKGATE = 0x5,
    IDTFLAGS16B_INTRGATE = 0x6,
    IDTFLAGS16B_TRPGATE = 0x7,
    IDTFLAGS32B_INTRGATE = 0xE,
    IDTFLAGS32B_TRPGATE = 0xF,

    IDTFLAGS_RING0 =     (0 << 5),
    IDTFLAGS_RING1 =     (1 << 5),
    IDTFLAGS_RING2 =     (2 << 5),
    IDTFLAGS_RING3 =     (3 << 5),
    
    IDTFLAGS_PRESENT = 0x80
}IDTFLAGS;

typedef struct InterruptFrame{
    uint32_t eip;
    uint16_t cs;
    uint32_t flags;
    uint32_t sp,
    uint16_t ss;
}__attribute__((packed)) InterruptFrame;

typedef struct RegisterFrame{
    uint32_t eax,
             ecx,
             edx,
             ebx,
             esp,
             ebp,
             esi,
             edi;
}__attribute__((packed)) RegisterFrame;

extern void __attribute__((cdecl)) LoadIDT(IDTdesc *ptr);
void __attribute__((cdecl)) isr_handlerC(InterruptFrame *IFrame, RegisterFrame *RFrame);
void __attribute__((cdecl)) SetInterrupt(uint32_t interrupt, 
    bool present,
    void *base,
    uint216_t segDesc,
    uint8_t flags
);
