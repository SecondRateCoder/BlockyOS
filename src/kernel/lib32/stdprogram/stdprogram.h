#pragma once

#include "./kernel/lib32/stdfile/stdfile.h"
#include "./kernel/lib32/stdfile/f-rat.h"
#include "./kernel/lib32/stdio/stdio.h"
#include "./kernel/lib32/stdkernel/GDT/GDT.h"
#include "./kernel/lib32/stdkernel/IDT/Interrupt.h"
#include "./kernel/public/kernpublic.h"

typedef struct standardHeader{
    uint8_t ID[10];
    FILE *program;
    void *CODE;
    uint8_t loadedCODEPages;
    void *DATA;
    uint8_t loadedDATAPages;
    struct standardChildren{
        stdfileENVIROMENT stdfile;
    }standardChildren;
}standardHeader;

uinl32_t loadedPrograms;
standardHeader **Programs;

standardHeader getCODEBase(void *CODE);
standardHeader getDATABase(void *DATA);