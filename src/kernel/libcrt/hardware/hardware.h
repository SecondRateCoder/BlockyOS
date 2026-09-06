#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

LibAPI bool TestMSR(void);
LibAPI uint64_t ReadMSR(uint32_t msr);
LibAPI void WriteMSR(uint32_t msr, uint64_t value);


