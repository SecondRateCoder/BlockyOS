#pragma once

#include "Interrupt/interrupt.h"
#include "Interrupt/IRQ/IRQ.h"
#include "GDT/GDT.h"
#include "IO/IO.h"
#include "kernel/lib32/stdmem/32/stdmem.h"

#define i868GDT_SEGCODE 0x08
#define i868GDT_SEGDATA 0x10

typedef struct BootIn{
    MemInfo Memory;
}BootIn;

void LoadSTDKernelState(gdtDESC_t *inGDT, idtDESC_t *inIDT, BootIn in, uint8_t PIC1InterruptLine, uint8_t PIC2InterruptLine);