#include "IDT.h"
#include "GDT.h"

gdtENTRY_t gdtTABLE16[] = {
    /* Null descriptor */
    { .limit = 0x0000, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x00, .granularity = 0x00, .base_high = 0x00 },

    /* 32-bit Code segment: limit=0xFFFF, base=0x00000000, access=0x9A, gran=0xCF */
    { .limit = 0xFFFF, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x9A, .granularity = 0xCF, .base_high = 0x00 },

    /* 32-bit Data segment: limit=0xFFFF, base=0x00000000, access=0x92, gran=0xCF */
    { .limit = 0xFFFF, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x92, .granularity = 0xCF, .base_high = 0x00 },

    /* 16-bit Code segment: limit=0x0000, base=0x00000000, access=0x9A, gran=0x0F */
    { .limit = 0x0000, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x9A, .granularity = 0x0F, .base_high = 0x00 },

    /* 16-bit Data segment: limit=0xFFFF, base=0x00000000, access=0x92, gran=0x0F */
    { .limit = 0xFFFF, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x92, .granularity = 0x0F, .base_high = 0x00 }
};

gdtDESC_t desc16 = {
    .address = gdtTABLE16,
    .limit = (uint16_t)(sizeof(gdtTABLE16))
};