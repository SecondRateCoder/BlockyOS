#pragma once

#include "./kernel/public/kernpublic.h"

typedef void __attribute__((cdecl)) (*interruptEntry)(void);
extern interruptEntry interruptTable[256];
extern __attribute__((packed)) uint8_t interruptTableEnd;
extern uint32_t interruptTableLength;

typedef struct IDTentry{
    uint16_t baseLow;
    uint16_t segDesc;
    uint8_t reserved;
    uint8_t flags;
    uint16_t baseHigh;
}__attribute__((packed)) IDTentry;

typedef struct IDTDesc{
    uint16_t size;
    uint32_t table;
}__attribute__((packed)) IDTDesc;

// Should be at address NULL, 0x00
IDTentry IDT[256];

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
    uint32_t ds, es;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t interrupt, error_code;
    uint32_t eip, cs, eflags, cpu_esp, ss;
}__attribute__((packed)) InterruptFrame;

extern void __attribute__((cdecl)) LoadIDT(IDTDesc *ptr);
void __attribute__((cdecl)) isr_handlerC(InterruptFrame *IFrame);
void __attribute__((cdecl)) InitInterrupt(
    uint32_t interrupt, 
    bool present,
    void *base,
    uint16_t segDesc,
    uint8_t flags
);
