#pragma once

#include "./kernel/lib32/public/kernpublic.h"
#include "InterruptRoutines.h"
#include "IRQ/IRQ.h"

char *ERRORS[];

typedef void ASMCALL (*interruptEntry)(void);
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

void InitIDT(IDTentry *IDT, uint16_t CodeSegment);
void ASMCALL int32enable(void);
void ASMCALL int32disable(void);


extern void ASMCALL LoadIDT(IDTDesc *ptr);
void ASMCALL isr_handlerC(InterruptFrame *IFrame);

void ToggleInterrupt(IDTentry *IDT, uint8_t interrupt, bool present);
void InitInterrupt(
    IDTentry *IDT, 
    uint32_t interrupt, 
    bool present,
    void *base,
    uint16_t segDesc,
    uint8_t flags
);
