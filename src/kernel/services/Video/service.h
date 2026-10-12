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

//	(uint8_t)RED, (uint8_t)BLUE, (uint8_t)GREEN, (uint8_t)ALPHA
typedef uint32_t Pixel;

//  Each bit corresponds to a Single Unit of Video
typedef struct{
	uint32_t	Matcher;
	//	The width of the Glyph, in bits.
	//		Must be rounded to 8.
	uint16_t	BitWidth, BitHeight;
	//	The number of empty units that should be placed before actually printing the Glyph
	uint16_t	VerticalStride, HorizontalStride;
	uint8_t		Data[];
}FontGlyph;

enumdef(char, TextCopyFFormat){
	FormatStart = '%', FormatIntegerBase = '.', 
	FormatUInteger8 = 'u', FormatUInteger16 = 'h', FormatUInteger32 = 'l', FormatUInteger64 = 'z', 
	FormatInteger8 = 'o', FormatInteger16 = 'e', FormatInteger32 = 'r', FormatInteger64 = 'i', 
	FormatString = 's', FormatChar = 'c'
};

void TextCopyF(char *stream, uint64_t *size, const char **f, va_list ls);
LibAPI uint64_t PrintGlyph(void *Memory, uint64_t BufferWidth, FontGlyph Glyph, void *Unit, uint32_t UnitSize);
LibAPI void PrintTextByGlyph(void *Memory, uint64_t *Counter, uint64_t BufferWidth, FontGlyph **GlyphTable, uint32_t TableLength, void *Unit, uint32_t UnitSize, char *Text);