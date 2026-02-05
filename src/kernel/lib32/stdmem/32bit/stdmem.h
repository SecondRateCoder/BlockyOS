#pragma once

#include "./kernel/lib32/generic/standard.h"

typedef struct E820MemBlock{
    size_t  Base,
            Limit;
    uint32_t Type,
             ACPI;
}PACKEDSTRUCT E820MemBlock;

typedef struct MemRegion{
    size_t Base,
           Limit;
    uint32_t Type,
             ACPI;
}MemRegion;

typedef struct MemInfo{
    uint32_t Count;
    union{
        MemRegion *regions;
        E820MemBlock *E820regions;
    };
}MemInfo;

