#pragma once

#include "./kernel/public/public/math/math.h"

extern void __attribute__((cdecl)) LoadIDT(void *ptr);
void __attribute__((cdecl)) isr_handlerC();