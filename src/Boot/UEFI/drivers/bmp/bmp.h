#pragma once

#include "src/Boot/UEFI/standard.h"
#include "src/Boot/UEFI/tools/tools.h"
#include "src/Boot/UEFI/drivers/socket/socket.h"

typedef struct{
	union{
		char		_BM[2];
		uint16_t	Magic;
	}magic;
	UINT32			FileByteSize;
	UINT16			Rsv[2];
	UINT32			ImageDataOffset;
}__packed BMPHeader_t;

enumdef(UINT32, DIBHeaderSize, DIBHeaderType){
	BITMAPCOREHEADER = 0x0C, BITMAPINFOHEADER = 0x28, 
	BITMAPV4HEADER = 0x6C, BITMAPV5HEADER = 0x7C
};

typedef struct{
	UINT32			bcSize;
	UINT16			bcWidth, 
					bcHeight, 
					bcPlanes, 
					bcPixelBitCount;
}__packed BITMAPCOREHEADER_t;


enumdef(UINT32, BITMAPINFOHEADERbiCompression){
	BI_RGB  = 0x00, BI_RLE8 = 0x01, BI_RLE4 = 0x02, BI_BITFIELDS = 0x03, 
	BI_JPEG = 0x04, BI_PNG = 0x05, BI_CMYK = 0x0B, BI_CMYKRLE8 = 0x0C, BI_CMYKRLE4 = 0x0D
};
typedef struct{
	UINT32							biSize; 
	INT32							biWidth, 
									biHeight;
	UINT16							biPlanes, 
									biBitCount;
	BITMAPINFOHEADERbiCompression	biCompression; 
	UINT32							biSizeImage;
	INT32							biXPelsPerMeter, 
									biYPelsPerMeter;
	UINT32							biClrUsed, 
									biClrImportant;
}__packed BITMAPINFOHEADER_t;

enumdef(UINT32, BITMAPV4HEADERbV4CSType, BITMAPV4HEADERbV5CSType){
	LCS_CALIBRATED_RGB = 0, 
	LCS_sRGB = 0x73524742, 
	LCS_WINDOWS_COLOR_SPACE = 0x57696E20, 

	PROFILE_LINKED = 0x4C494E4B, 
	PROFILE_EMBEDDED = 0x4D424544
};

enumdef(UINT32, BITMAPV5HEADERbV5Intent){
	LCS_GM_BUSINESS = 0x01, LCS_GM_GRAPHICS = 0x02, 
	LCS_GM_IMAGES = 0x04, LCS_GM_ABS_COLORIMETRIC = 0x08
};
typedef struct{
	BITMAPINFOHEADER_t		INFOHEADER;
	UINT32					bV4RedMask, 
							bV4GreenMask, 
							bV4BlueMask, 
							bV4AlphaMask;
	union{
		BITMAPV4HEADERbV4CSType	bV4CSType;
		BITMAPV4HEADERbV5CSType	bV5CSType;
	};
	UINT64					bV4Endpoints[4];
	UINT32					bV4GammaRed, 
							bV4GammaGreen, 
							bV4GammaBlue;
}__packed BITMAPV4HEADER_t;

typedef struct{
	BITMAPV4HEADER_t		_V4;
	BITMAPV5HEADERbV5Intent	bV5Intent;
	UINT32					bV5ProfileData, 
							bV5ProfileSize, 
							bV5Reserved;
}__packed BITMAPV5HEADER_t;

typedef struct{
	void					*Raw;
	BMPHeader_t				*Header;
	UINT32					Type;
	union{
		BITMAPCOREHEADER_t	*Core;
		BITMAPINFOHEADER_t	*Info;
		BITMAPV4HEADER_t	*V4;
		BITMAPV5HEADER_t	*V5;
	};
	void					*ImageData;
	UINT32					Width,
							Height,
							PixelSize;
}__packed BMPUPackedHeader_t;

BMPUPackedHeader_t *OpenBMP(socket_t *bmp, EFI_GRAPHICS_PIXEL_FORMAT PixelFormat);
void FormatBMPImageData(BMPUPackedHeader_t *bmp, EFI_GRAPHICS_PIXEL_FORMAT PixelFormat);
void CloseBMP(BMPUPackedHeader_t *bmp);