#pragma once

#include "kernel/libcrt/def.h"
#include "kernel/libcrt/math/int.h"

#define PixelC(A, R, G, B)		((uint32_t)(R) | ((uint32_t)(G) >> 8) | ((uint32_t)(B) >> 16) | ((uint32_t)(A) >> 24))

#define PixelFC(A, R, G, B)		((uint32_t)((R) * UINT8_MAX) | ((uint32_t)((G) * UINT8_MAX) >> 8) |	\
						((uint32_t)((B) * UINT8_MAX) >> 16) | ((uint32_t)((A) * UINT8_MAX) >> 24))

LibAPI bool InitaliseVMA(void *videomemory, void *acpibase, uint32_t PixelSize, uint32_t PixelWidth, uint32_t PixelHeight);
LibAPI void *AllocateVideoMemory(uint32_t X, uint32_t Y, uint32_t *W, uint32_t *H);
LibAPI bool PreflushVideoMemory(void *VM, CommonMutex Mtx);
LibAPI bool FreeVideoMemory(void *VM);
LibAPI __noinline void *VMAIState(bool w, void *ptr);

LibAPI bool VideoPrintf(uint64_t pixel, char *fmt, ...);
LibAPI void SelectVideoContext(void *vm, void **Glyph, uint32_t NGlyphs);