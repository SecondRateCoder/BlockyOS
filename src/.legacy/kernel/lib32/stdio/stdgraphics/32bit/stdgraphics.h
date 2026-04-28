#include "kernel/lib32/generic/standard.h"

#define toIndex(x, y, maxX) ((y * maxX) + x)

#define sizeToMask(size)(																							\
	((size) <= 1? 0x1: 0) | ((size) <= 1? 0x1: 0) | ((size) <= 2? 0x2: 0) | ((size) <= 3? 0x4: 0) | ((size) <= 4? 0x8: 0)		\
	 | ((size) <= 5? 0x10: 0) | ((size) <= 6? 0x20: 0) | ((size) <= 7? 0x40: 0) | ((size) <= 8? 0x80: 0)					\
)

#define packPixel(a, r, g, b, aSize, aOffset, rSize, rOffset, gSize, gOffset, bSize, bOffset)	\
(((((uint32_t)(a)) | sizeToMask(aSize)) << (aOffset)) | ((((uint32_t)(r)) | sizeToMask(rSize)) << (rOffset)) | 	\
((((uint32_t)(g)) | sizeToMask(gSize)) << (gOffset)) | ((((uint32_t)(b)) | sizeToMask(bSize)) << (bOffset)))

#define fontCode(a, b, c, d) (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))

typedef struct VBEModeBlockHeader{
	struct VESA0_0{
		uint16_t modeattributes;
		uint8_t WindowAattrributes,
				WindowBattributes;
		uint16_t WindowGranularityinKB,
				 WindowSizeinKB;
		uint16_t WinAstartingSegment,
				 WinBstartingSegment;
		void *WinPtr_VideoMemory;
		uint16_t BytesperScanLine;
	}VESA0_0;
	struct VESA1_0{
		uint16_t width,
				 height;
		uint8_t PixelWidthperChar,
				PixelHeightperChar;
		uint8_t num_memPlanes,
				bits_perpixel,
				num_banks,
				memoryModel,
				bankSizeinKB,
				maxImagePages_minusOne,
				padding;
	}VESA1_0;
	struct  VESA1_2{
		uint8_t redMaskSize,
				redFieldOffset,
				greenMaskSize,
				greenFieldOffset,
				blueMaskSize,
				blueFieldOffset,
				reservedMaskSize,
				reservedFieldOffset,
				directColorModeInfo;
	}VESA1_2;
	struct VESA2_0{
		void *linearFrameBuffer,
			 *offscreenMemory;
		uint16_t offscreenMemorySizeinKB;

	}VESA2_0;
	struct VESA3_0{
		uint16_t BytesperScanLine;
		uint8_t ImagesperBankedModeminusOne,
				ImagesperLinearModeminusOne;
		uint8_t directRedMaskSize,
				directRedMaskOffset,
				directGreenMaskSize,
				directGreenMaskOffset,
				directBlueMaskSize,
				directBlueMaskOffset,
				directReservedMaskSize,
				directReservedMaskOffset;
		uint32_t maxPixelClock;
	}VESA3_0;
}PACKEDSTRUCT VBEModeBlockHeader;

typedef struct VBEModeBlock{
	VBEModeBlockHeader header;
	uint8_t reserved[256 - sizeof(VBEModeBlockHeader)];
}VBEModeBlock;

typedef union Pixel{
	uint32_t Pixel;
	struct unPacked{
		uint8_t A, R, G, B;
	}unPacked;
}Pixel;

#define CharImageSize 32
#define FontCodeSize 6
typedef struct Char{
	char c;
	uint8_t image[CharImageSize];
}Char;
typedef struct Font{
	char code[FontCodeSize];
	uint32_t tablesize;
	uint8_t width, height;
	Char table[];
}Font;

#define MAXFONTS 8
typedef struct FontTable{
	Font fonts[MAXFONTS];
}FontTable;

Char *getFontCharF(Font *f, char c);
Font *getFont(char code[FontCodeSize]);
void setFontChar(Char c, char code[FontCodeSize]);
Char *getFontChar(char code[FontCodeSize], char c);
void putimage(Pixel *image, Pixel normal, float mix, uint8_t width, uint8_t height, uint32_t x, uint32_t y);
bool putcdecoded(char c, Pixel colorOffset, uint32_t *x, uint32_t *y, Font *f);

static const Font PlainBasic;