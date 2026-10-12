#include "bmp.h"

void FormatBMPImageData(BMPUPackedHeader_t *bmp, EFI_GRAPHICS_PIXEL_FORMAT PixelFormat){
	if(!bmp || !bmp->ImageData || bmp->Width == 0 || bmp->Height == 0){ return; }
	INT32 rawHeight = (INT32)bmp->Height;
	UINT32 absHeight = (rawHeight < 0) ? (UINT32)(-rawHeight) : (UINT32)rawHeight, 
			width = bmp->Width, 
			bpp = bmp->PixelSize; // Source bytes per pixel

	//	BMP scanlines are padded to 4-byte boundaries
	UINT32 srcRowStride = ((width * bpp + 3) & ~3);

	//	Allocate 32bpp output pixel buffer
	UINT32 *formattedData = (UINT32 *)__calloc(width * absHeight, sizeof(UINT32));
	if(!formattedData){return;}
	UINT8 *srcBytes = (UINT8 *)bmp->ImageData;

	for(register UINT32 y = 0; y < absHeight; y++){
		// BMP is bottom-up if Height > 0, top-down if Height < 0
		UINT32 srcY = (rawHeight > 0) ? (absHeight - 1 - y) : y;
		UINT8 *srcRow = srcBytes + (srcY * srcRowStride);
		UINT32 *dstRow = formattedData + (y * width);
		for(register UINT32 x = 0; x < width; x++){
			UINT8 r = 0, g = 0, b = 0, a = 0xFF;
			if(bpp == 3){//	24-bit BGR
				b = srcRow[x * 3 + 0];
				g = srcRow[x * 3 + 1];
				r = srcRow[x * 3 + 2];
			}else if(bpp == 4){//	32-bit BGRA
				b = srcRow[x * 4 + 0];
				g = srcRow[x * 4 + 1];
				r = srcRow[x * 4 + 2];
				a = srcRow[x * 4 + 3];
			}else if(bpp == 1){r = g = b = srcRow[x];}//	8-bit Grayscale
			// Memory order: [R, G, B, Reserved]
			if(PixelFormat == PixelRedGreenBlueReserved8BitPerColor){
				dstRow[x] = ((UINT32)a << 24) | ((UINT32)b << 16) | ((UINT32)g << 8) | (UINT32)r;
			// Default / PixelBlueGreenRedReserved8BitPerColor: [B, G, R, Reserved]
			}else{dstRow[x] = ((UINT32)a << 24) | ((UINT32)r << 16) | ((UINT32)g << 8) | (UINT32)b;}
		}
	}

	// Swap raw image bytes with 32bpp formatted pixel buffer
	__free(bmp->ImageData);
	bmp->ImageData = formattedData;
	bmp->PixelSize = 4; // Normalized to 32bpp
	bmp->Height = absHeight;
}

BMPUPackedHeader_t *OpenBMP(socket_t *bmp, EFI_GRAPHICS_PIXEL_FORMAT PixelFormat){
	socket_ret rt = {0};
	rt = socketfcall(bmp, read, 0, sizeof(BMPHeader_t) + sizeof(BITMAPV5HEADER_t));
	if(socketreterr(rt, sizeof(BMPHeader_t) + sizeof(BITMAPV5HEADER_t))){
		DEBUGPRINT(L"\nCant read Bytes from file");		return NULL;}
	BMPHeader_t *Hdr = rt.data;
	BITMAPCOREHEADER_t *Core = rt.data + sizeof(BMPHeader_t);
	if(Hdr->magic._BM[0] == 'B' && Hdr->magic._BM[1] == 'M'){
		BMPUPackedHeader_t *Out = __calloc(1, sizeof(BMPUPackedHeader_t));
		Out->Header = __memdup(Hdr, sizeof(BMPHeader_t));
		Out->Type = Core->bcSize;
		Out->Core = __memdup(Core, Out->Type);
		__free(rt.data);
		switch(Out->Type){
			case BITMAPCOREHEADER: {
				Out->Width = Out->Core->bcWidth;
				Out->Height = Out->Core->bcHeight;
				Out->PixelSize = __roundup(Out->Core->bcPixelBitCount, 8) / 8;
				break;
			} case BITMAPINFOHEADER: {
				Out->Width = Out->Info->biWidth;
				Out->Height = Out->Info->biHeight;
				Out->PixelSize = __roundup(Out->Info->biBitCount, 8) / 8;
				break;
			} case BITMAPV4HEADER: {
				Out->Width = Out->V4->INFOHEADER.biWidth;
				Out->Height = Out->V4->INFOHEADER.biHeight;
				Out->PixelSize = __roundup(Out->V4->INFOHEADER.biBitCount, 8) / 8;
				break;
			} case BITMAPV5HEADER: {
				Out->Width = Out->V5->_V4.INFOHEADER.biWidth;
				Out->Height = Out->V5->_V4.INFOHEADER.biHeight;
				Out->PixelSize = __roundup(Out->V5->_V4.INFOHEADER.biBitCount, 8) / 8;
				break;
			}
		}
		//	Format the Image
		// Read raw bitmap payload accounting for 4-byte row stride padding
		UINT64 totalBytes = ((Out->Width * Out->PixelSize + 3) & ~3) * (((INT32)Out->Height < 0)? (UINT32)(-(INT32)Out->Height): Out->Height);
		rt = socketfcall(bmp, read, Out->Header->ImageDataOffset, totalBytes);
		Out->ImageData = rt.data;

		// Format the Image to target EFI Framebuffer layout
		FormatBMPImageData(Out, PixelFormat);
		return Out;
	}
	return NULL;
}

void CloseBMP(BMPUPackedHeader_t *bmp){
	__free(bmp->Header);
	__free(bmp->Core);
	__free(bmp->ImageData);
	__free(bmp);
}