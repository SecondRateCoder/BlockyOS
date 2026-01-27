#pragma once

#include "./kernel/public/public/math/math.h"

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

extern void __attribute__((cdecl)) LoadIDT(void *ptr);
void __attribute__((cdecl)) isr_handlerC(InterruptFrame *IFrame, RegisterFrame *RFrame);
