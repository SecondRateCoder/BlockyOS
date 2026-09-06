#pragma once

#include "XSDT.h"

//	The Boot Graphics Record Table (BGRT) is an optional ACPI table which contains a pointer to the image that has been drawn during boot. 
#define XSDT_BGRTCode	"BGRT"

enumdef(uint8_t, XSDT_BGRTStatus){
	XSDT_Displayed = 0x1, 
		XSDT_NoOrientation = (0x00 << 1), XSDT_Orientation90 = (0x01 << 1), 
		XSDT_Orientation180 = (0x02 << 1), XSDT_Orientation270 = (0x03 << 1)
};
enumdef(uint8_t, XSDT_BGRTImageType){XSDT_Bitmap = 0x1};

typedef struct{
	SDTHeader_t     		Header;
	//	Must be 1
	uint16_t				VersionID;
	XSDT_BGRTStatus			Status;
	XSDT_BGRTImageType		ImageType;
	//	64-bit physical address to an in-memory copy of the displayed image. 
	//	It can be a 24 bit bitmap (0xRRGGBB) or a 
	//		32 bit bitmap (0xrrRRGGBB), where lowercase R means reserved. 
	uint64_t				ImageAddress;
	uint32_t				ImageXOffset;
	uint32_t				ImageYOffset;
}XSDT_BGRT_t;

typedef struct{
	//	Must be 0x4D42 ('BM' in little-endian)
    uint16_t Signature;
	// Total BMP file size in bytes
    uint32_t Size;
    uint16_t Reserved0[2];
	//	Byte offset to raw pixel data
    uint32_t ImageOffset;
}__packed BMPFileHeader;

// Standard DIB / BITMAPINFOHEADER (40 bytes)
typedef struct{
	//	Size of this header (usually 40)
    uint32_t HeaderSize;
	//	Image width in pixels
    int32_t  Width;
	//	Image height in pixels (can be negative!)
    int32_t  Height;
	//	Always 1
    uint16_t Planes;
	//	24 (0xRRGGBB) or 32 (0xrrRRGGBB)
    uint16_t BitsPerPixel;
	//	Should be 0 (BI_RGB)
    uint32_t Compression;
	//	Size of raw pixel data (may be 0 for BI_RGB)
    uint32_t ImageSize;
    int32_t  XpixelsPerM;
    int32_t  YpixelsPerM;
    uint32_t ColorsUsed;
    uint32_t ImportantColors;
}__packed BMPInfoHeader;