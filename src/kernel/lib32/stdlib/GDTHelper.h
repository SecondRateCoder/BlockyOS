#pragma once

#include "./kernel/public/public/math/math.h"

typedef struct gdtENTRY_t{
    union{
        struct{
            uint16_t limit;
            uint16_t base_low;
            uint8_t  base_mid;
            uint8_t  access;
            uint8_t  granularity;
            uint8_t  base_high;
        };
        size_t ENTRY;
    };
}__attribute__((packed)) gdtENTRY_t;

typedef struct gdtDESC_t{
    // Byte size of GDT table
    uint16_t limit;

    // Address of GDT table
    gdtENTRY_t  *address;
}__attribute__((packed)) gdtDESC_t;

gdtDESC_t GDTdesc;
gdtENTRY_t gdtTABLE16[];

extern void __attribute__((cdecl)) LoadGDT(gdtDESC_t *);