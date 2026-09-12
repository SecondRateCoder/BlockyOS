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

void PrintTextByGlyph(void *Memory, uint64_t BufferWidth, FontGlyph **GlyphTable, uint32_t TableLength, void *Unit, uint32_t UnitSize, char *Text){
	uint64_t Offset = 0;
	for(uint32_t c = 0; c < strlen(Text); ++c){
		for(uint32_t cc = 0; cc < TableLength; ++cc){
			if(Text[c] == GlyphTable[cc]->Matcher){
				PrintGlyph(Memory + Offset, BufferWidth, *(GlyphTable[cc]), Unit, UnitSize);
			}
		}
	}
}

#include "kernel/libcrt/memory/string.h"

enumdef(char, TextCopyFFormat){
	FormatStart = '%', FormatIntegerBase = '.', 
	FormatUInteger8 = 'u', FormatUInteger16 = 'h', FormatUInteger32 = 'l', FormatUInteger64 = 'z', 
	FormatInteger8 = 'o', FormatInteger16 = 'e', FormatInteger32 = 'r', FormatInteger64 = 'i', 
	FormatString = 's', FormatChar = 'c'
};

// void TextCopyF(char *stream, uint32_t size, char *format, ...){
// 	uint64_t textlen = strlen(format);
// 	va_list ls;		va_start(ls, format);
// 	static const char sample[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 
// 		'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 
// 		'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
// 	for(uint64_t counter = 0x00, cc = 0; (cc < textlen) && (counter < size); ++cc, ++counter){
// 		switch(format[cc]){
// 			case FormatStart: {
// 				for(; (cc < textlen) && (counter < size); ++cc){
// 					uint64_t base = 10, value = 0;
// 					bool formattype = 0x00;
// 					switch(format[cc]){
// 						case FormatIntegerBase: {
// 							base = tolonglong(format + (cc++));
// 							base = __max(base, sizeof(sample));
// 							while((format[cc] >= '0') && (format[cc] <= '9')){cc++;}
// 							break;
// 						} 
// 						case FormatUInteger8:	{value = (uint64_t)va_arg(ls, uint8_t);		formattype = FormatUInteger64;	break;} 
// 						case FormatUInteger16:	{value = (uint64_t)va_arg(ls, uint16_t);	formattype = FormatUInteger64;	break;} 
// 						case FormatUInteger32:	{value = (uint64_t)va_arg(ls, uint32_t);	formattype = FormatUInteger64;	break;} 
// 						case FormatUInteger64:	{value = (uint64_t)va_arg(ls, uint64_t);	formattype = FormatUInteger64;	break;} 
// 						case FormatInteger8:	{int8_t temp = va_arg(ls, int8_t);		value = (uint64_t)*((uint64_t *)&temp);		formattype = FormatInteger64;	break;} 
// 						case FormatInteger16:	{int16_t temp = va_arg(ls, int16_t);	value = (uint64_t)*((uint64_t *)&temp);		formattype = FormatInteger64;	break;} 
// 						case FormatInteger32:	{int32_t temp = va_arg(ls, int32_t);	value = (uint64_t)*((uint64_t *)&temp);		formattype = FormatInteger64;	break;} 
// 						case FormatInteger64:	{int64_t temp = va_arg(ls, int64_t);	value = (uint64_t)*((uint64_t *)&temp);		formattype = FormatInteger64;	break;} 
// 						case FormatString: {
// 							char *str = va_arg(ls, char *);
// 							uint64_t len = strlen(str);
// 							if(len > (size - counter)){continue;}
// 							memcpy(stream + counter, str, len);
// 							break;
// 						} case FormatChar: {stream[counter] = va_arg(ls, char);		break;}
// 					}
// 					switch(formattype){
// 						case FormatInteger64: {
// 							int64_t temp = *((uint64_t *)&value);
// 							if(temp < 0){stream[counter] = '-';		counter++;}
// 						} case FormatUInteger64: {
// 							while(value && (counter < size)){
// 								uint64_t rem = value % base;
// 								stream[counter] = sample[rem];
// 								value /= base;
// 								counter++;
// 							}
// 							break;
// 						} default: {continue;}
// 					}
// 				}
// 				break;
// 			} default: {stream[counter] = format[cc];	counter++;}
// 		}
// 	}
// }

void TextCopyF(char *stream, uint64_t *size, const char **format, va_list ls){
    if(!stream || (*size) == 0 || !format || !ls){return;}

    static const char sample[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    uint32_t counter = 0;
    
    //	Reserve 1 byte at the end for the null terminator (size - 1)
    for(uint32_t cc = 0; (*format)[cc] != '\0' && counter < ((*size) - 1); ++cc){
        
        if((*format)[cc] == FormatStart){
            cc++; // Move past the start token
            if((*format)[cc] == '\0'){break;}

            uint64_t base = 10, 
					value = 0;
            bool is_signed = false, 
				negative = false;

            //	SAFELY EXTRACT VARIADIC ARGUMENTS
            // Smaller integer types (char, short) are automatically promoted to int/unsigned int by the C compiler.
            switch((*format)[cc]){
                case FormatUInteger8:	{value = (uint64_t)(uint8_t) va_arg(ls, unsigned int);	break;}
                case FormatUInteger16:	{value = (uint64_t)(uint16_t)va_arg(ls, unsigned int);	break;}
                case FormatUInteger32:	{value = (uint64_t)va_arg(ls, uint32_t);				break;}
                case FormatUInteger64:	{value = va_arg(ls, uint64_t);							break;}
                case FormatInteger8: {
                    int8_t temp = (int8_t)va_arg(ls, int);
                    if (temp < 0) { negative = true; value = (uint64_t)(-temp); }
                    else { value = (uint64_t)temp; }
                    is_signed = true;
                    break;
                } case FormatInteger16: {
                    int16_t temp = (int16_t)va_arg(ls, int);
                    if (temp < 0) { negative = true; value = (uint64_t)(-temp); }
                    else { value = (uint64_t)temp; }
                    is_signed = true;
                    break;
                } case FormatInteger32: {
                    int32_t temp = va_arg(ls, int32_t);
                    if (temp < 0) { negative = true; value = (uint64_t)(-temp); }
                    else { value = (uint64_t)temp; }
                    is_signed = true;
                    break;
                } case FormatInteger64: {
                    int64_t temp = va_arg(ls, int64_t);
                    if (temp < 0) { negative = true; value = (uint64_t)(-temp); }
                    else { value = (uint64_t)temp; }
                    is_signed = true;
                    break;
                } case FormatString: {
                    const char *str = va_arg(ls, const char *);
                    if (!str) str = "(null)";
                    // Correctly track the string copy lengths
                    while (*str && counter < ((*size) - 1)) {
                        stream[counter++] = *str++;
                    }
                    continue; // Skip the integer rendering logic below
                } case FormatChar: {
                    stream[counter++] = (char)va_arg(ls, int);
                    continue; 
                } default: {stream[counter++] = (*format)[cc];        continue;}//	If the token is unrecognized, print it as a literal
				
            }

            //	FORMAT INTEGER TO TEMPORARY BUFFER
            //	Modulo arithmetic extracts the LEAST significant digit first. 
            //	We store it in a temp buffer so we can reverse it.
            char num_buf[70]; 
            int num_len = 0;

			//	Handle explicit zero
            if(value == 0){num_buf[num_len++] = '0';}else{
                while(value > 0){
                    num_buf[num_len++] = sample[value % base];
                    value /= base;
                }
            }
            if(negative && is_signed){num_buf[num_len++] = '-';}

            //	COPY TO STREAM IN REVERSE ORDER
            for(int i = num_len - 1; i >= 0 && counter < ((*size) - 1); i--){stream[counter++] = num_buf[i];}

		//	Normal character processing
        }else{stream[counter++] = (*format)[cc];}
    }

    //	GUARANTEE NULL TERMINATION
    stream[counter] = '\0';
    
    va_end(ls);
}