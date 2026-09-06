#pragma once
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/services.h"
#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/hardware/IDT/ISR.h"
#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/memory/string.h"
#include "kernel/libcrt/hardware/PCIe/PCIe.h"
#include "kernel/libcrt/hardware/PCIe/Devices.h"
#include "kernel/libcrt/memory/allocator/malloc.h"

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

uint64_t PrintGlyph(void *Memory, uint64_t BufferWidth, FontGlyph Glyph, void *Unit, uint32_t UnitSize){
    // Calculate starting position including strides
    uint8_t *PixelMem = (uint8_t *)Memory + (UnitSize * (Glyph.HorizontalStride + (Glyph.VerticalStride * BufferWidth)));
    uint32_t bytesPerRow = (Glyph.BitWidth + 7) / 8;
    for(uint32_t y = 0; y < Glyph.BitHeight; ++y){
        uint8_t *RowMem = PixelMem; // Save start of this row
        for(uint32_t x = 0; x < Glyph.BitWidth; ++x){
            uint32_t byteIdx = (y * bytesPerRow) + (x / 8);
            uint8_t bitIdx = 7 - (x % 8); // Assuming MSB-first font layout
            if((Glyph.Data[byteIdx] >> bitIdx) & 1){memcpy(RowMem, Unit, UnitSize);}
            RowMem += UnitSize;
        }
        // Move down one full scanline width in the frame buffer
        PixelMem += (BufferWidth * UnitSize);
    }
	return (uint64_t)PixelMem - (uint64_t)Memory;
}

void PrintTextByGlyph(void *Memory, uint64_t BufferWidth, FontGlyph *GlyphTable, uint32_t TableLength, void *Unit, uint32_t UnitSize, char *Text){
	uint64_t Offset = 0;
	for(uint32_t c = 0; c < strlen(Text); ++c){
		for(uint32_t cc = 0; cc < TableLength; ++cc){
			if(Text[c] == GlyphTable[cc].Matcher){
				PrintGlyph(Memory + Offset, BufferWidth, GlyphTable[cc], Unit, UnitSize);
			}
		}
	}
}