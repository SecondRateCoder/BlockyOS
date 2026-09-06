#pragma once

#include "kernel/libcrt/def.h"
#include "kernel/libcrt/math/int.h"

bool InitaliseVMA(void *videomemory, void *acpibase, uint32_t PixelSize, uint32_t PixelWidth, uint32_t PixelHeight);
void *AllocateVideoMemory(uint32_t X, uint32_t Y, uint32_t *W, uint32_t *H);
bool PreflushVideoMemory(void *VM, CommonMutex Mtx);
bool FreeVideoMemory(void *VM);