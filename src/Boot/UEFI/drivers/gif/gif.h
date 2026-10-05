#pragma once

// #include "src/Boot/UEFI/guid.h"
#include "src/Boot/UEFI/standard.h"
#include "src/Boot/UEFI/tools/tools.h"
#include "src/Boot/UEFI/drivers/socket/socket.h"

#define GlobalColorMapLength(N)	(2 << ((N) + 1))
#define LocalColorMapLength		GlobalColorMapLength

typedef struct{
	UINT8	Signature[3];
	UINT8	Version[3];
}__packed gifheader_t;
typedef struct{
	UINT16	ScreenWidth;
	UINT16	ScreenHeight;
	//	The Color Depth of the File - 1, 
	//	The number of GlobalColors = 2ColorDepth
	UINT8	BitsPerPixel				: 3;
	UINT8								: 1;
	//	significant bits per color in GlobalColorMap
	UINT8	ColorResolutionBitsMinusOne	: 3;
	//	0:	if default map is used, or if every image has a LocalColorMap
	//	1:	if GlobalColorMap exists (should be true in almost all cases)
	UINT8	GlobalColorMapEnable		: 1;
	UINT8	BackgroundColor;
	//	Usually 0
	//	The Actual Aspect Ratio is usually: (gifheader_t.AspectRatio + 15) / 64
	UINT8	AspectRatio;
}__packed gifLogicalScreenSecriptor_t;

//	repeated 2 to the power of (gifheader_t.BitsPerPixel + 1) times
//	Present only when GlobalDescriptor.GlobalColorMap = 1
typedef struct{
	UINT8	Red, Green, Blue;
}__packed gifGlobalColor_t, gifGlobalColorMap_t[];

enumdef(UINT8, gifExtensionBlockFunctionCode){
	PlaintextExtension = 0x01, LocalDescriptorExtension = 0xF9, 
	CommentExtension = 0xFE, ApplicationExtension = 0xFF
};
#define gifExtensionBlockMagic	(0x21)
typedef struct{
	//	0x21
	UINT8	Header;
	UINT8	FunctionCode;
	UINT8	DataLength;
	UINT8	Data[];
}__packed gifExtensionBlock_t;
enumdef(UINT8, gifExtBlockLocalDescriptorExtensionDataUndraw){RestoreBackground, RestorePrevious};
typedef struct{
	UINT8	TransparentColorExists	: 1;
	UINT8	UserInput				: 1;
	UINT8	Disposal				: 3;
	UINT8							: 3;
	UINT16	Duration;
	UINT8	TransparentColor;
}__packed gifExtBlockLocalDescriptorExtensionData;

#define gifLocalImageDescriptorMagic	(0x2C)
typedef struct{
	//	0x2C
	UINT8	Header;
	UINT16	HorizontalPosition;
	UINT16	VerticalPosition;
	UINT16	ImageWidth;
	UINT16	ImageHeight;
	//	The Color Depth of the Local - 1, 
	//	The number of LocalColors = (2ColorDepth)
	UINT8	BitsPerPixel		: 3;
	UINT8						: 2;
	UINT8	Sorted				: 1;
	UINT8	InterlacedImage		: 1;
	UINT8	LocalColorMapExists	: 1;
}__packed gifLocalImageDescriptor_t;

//	repeated 2 to the power of (gifLocalImageDescriptor_t.BitsPerPixel + 1) times
//	Present only when gifLocalImageDescriptor_t.GlobalColorMap = 1
typedef struct{
	UINT8	Red, Green, Blue;
}__packed gifLocalColor_t, gifLocalColorMap_t[];

typedef struct{
	UINT8	InitialCodeSize;
	UINT8	Length;
	UINT8	Data[];
}__packed gifRasterDataBlock_t;

typedef struct{
	gifLocalImageDescriptor_t	*ld;
	UINT32						lcmLength;
	gifLocalColorMap_t			*lcm;
	gifRasterDataBlock_t		*rdb;
	gifExtensionBlock_t			*gce; // Graphic Control Extension bound to this frame
}gifFrame_t;

typedef struct{
	void *Raw;
	gifheader_t *hdr;
	gifLogicalScreenSecriptor_t *lsd;
	UINT32 gcmLength;
	gifGlobalColorMap_t *gcm;
	UINT32 nExt;
	gifExtensionBlock_t **ext;

	UINT32 nFrames;
	gifFrame_t *frames;

	gifLocalImageDescriptor_t	*ld;
	UINT32						lcmLength;
	gifLocalColorMap_t			*lcm;
	gifRasterDataBlock_t		*rdb;
}gifDescriptionSpace_t;

gifDescriptionSpace_t *OpenGIF(socket_t *gif);
UINT32 *GetGIFFrame(gifDescriptionSpace_t *desc, UINT32 FrameIndex, UINT32 *_Width, UINT32 *_Height, EFI_GRAPHICS_PIXEL_FORMAT PixelFormat);
void BltGIFFrame(const UINT32 *FrameBuffer, UINT32 FrameWidth, UINT32 FrameHeight, UINT32 *DestBuffer, 
	INT32 DestX, INT32 DestY, UINT32 DestWidth, UINT32 DestHeight, UINT32 DestStride);