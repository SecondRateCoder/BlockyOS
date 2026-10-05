#include "gif.h"

gifDescriptionSpace_t *OpenGIF(socket_t *gif){
	socket_ret rt = {0};
	register UINT64 offset = 0;

	//	Read GIF Header
	rt = socketfcall(gif, read, offset, sizeof(gifheader_t));
	if(socketreterr(rt, sizeof(gifheader_t))){ return NULL; }
	offset += sizeof(gifheader_t);

	char *hdr_str = (char *)rt.data;
	if(!(hdr_str[0] == 'G' && hdr_str[1] == 'I' && hdr_str[2] == 'F')){
		__free(rt.data);
		return NULL;
	}

	gifDescriptionSpace_t *out = __calloc(1, sizeof(gifDescriptionSpace_t));
	out->hdr = (gifheader_t *)rt.data;

	//	Read Logical Screen Descriptor
	rt = socketfcall(gif, read, offset, sizeof(gifLogicalScreenSecriptor_t));
	if(socketreterr(rt, sizeof(gifLogicalScreenSecriptor_t))){ return out; }
	offset += sizeof(gifLogicalScreenSecriptor_t);
	out->lsd = (gifLogicalScreenSecriptor_t *)rt.data;

	//	Read Global Color Map (if present)
	if(out->lsd->GlobalColorMapEnable){
		UINT32 gcmCount = GlobalColorMapLength(out->lsd->BitsPerPixel);
		UINT64 gcmSize = sizeof(gifGlobalColor_t) * gcmCount;
		rt = socketfcall(gif, read, offset, gcmSize);
		if(socketreterr(rt, gcmSize)){ return out; }
		offset += gcmSize;
		out->gcmLength = gcmCount;
		out->gcm = (gifGlobalColorMap_t *)rt.data;
	}

	gifExtensionBlock_t *activeGCE = NULL;

	//	Parse GIF Stream Blocks
	while(true){
		UINT8 sentinel = 0;
		rt = socketfcall(gif, read, offset, sizeof(UINT8));
		if(socketreterr(rt, sizeof(UINT8))){ break; }
		sentinel = *((UINT8 *)rt.data);
		__free(rt.data);

		//	Extension Block (0x21)
		if(sentinel == gifExtensionBlockMagic){
			rt = socketfcall(gif, read, offset, sizeof(gifExtensionBlock_t));
			if(socketreterr(rt, sizeof(gifExtensionBlock_t))){ break; }
			
			gifExtensionBlock_t *hdrBlock = (gifExtensionBlock_t *)rt.data;
			UINT64 totalExtSize = sizeof(gifExtensionBlock_t) + hdrBlock->DataLength;
			offset += sizeof(gifExtensionBlock_t) + hdrBlock->DataLength;

			//	Read sub-blocks until length 0 byte
			while(true){
				rt = socketfcall(gif, read, offset, sizeof(UINT8));
				if(socketreterr(rt, sizeof(UINT8))){ break; }
				UINT8 subLen = *((UINT8 *)rt.data);
				__free(rt.data);
				offset += sizeof(UINT8);

				if(subLen == 0){ break; }
				offset += subLen;
				totalExtSize += sizeof(UINT8) + subLen;
			}

			//	Read full extension block memory
			rt = socketfcall(gif, read, offset - totalExtSize, totalExtSize);
			if(!socketreterr(rt, totalExtSize)){
				gifExtensionBlock_t *extBlock = (gifExtensionBlock_t *)rt.data;
				if(out->ext){
					out->ext = __realloc(out->ext, out->nExt * sizeof(gifExtensionBlock_t *), (out->nExt + 1) * sizeof(gifExtensionBlock_t *));
				}else{out->ext = __calloc(1, sizeof(gifExtensionBlock_t *));}
				out->ext[out->nExt++] = extBlock;

				//	Track if this is a Graphic Control Extension (0xF9) preceding a frame
				if(extBlock->FunctionCode == LocalDescriptorExtension){activeGCE = extBlock;}
			}
		}else if(sentinel == gifLocalImageDescriptorMagic){//	Local Image Descriptor (0x2C)
			rt = socketfcall(gif, read, offset, sizeof(gifLocalImageDescriptor_t));
			if(socketreterr(rt, sizeof(gifLocalImageDescriptor_t))){ break; }
			offset += sizeof(gifLocalImageDescriptor_t);
			
			gifLocalImageDescriptor_t *ld = (gifLocalImageDescriptor_t *)rt.data;
			gifLocalColorMap_t *lcm = NULL;
			UINT32 lcmLength = 0;

			//	Local Color Table (if present)
			if(ld->LocalColorMapExists){
				lcmLength = LocalColorMapLength(ld->BitsPerPixel);
				UINT64 lcmSize = sizeof(gifLocalColor_t) * lcmLength;
				rt = socketfcall(gif, read, offset, lcmSize);
				if(socketreterr(rt, lcmSize)){ break; }
				offset += lcmSize;
				lcm = (gifLocalColorMap_t *)rt.data;
			}

			//	LZW Raster Data Block
			UINT64 rdbStartOffset = offset;
			offset += sizeof(UINT8); // Skip InitialCodeSize byte

			while(true){
				rt = socketfcall(gif, read, offset, sizeof(UINT8));
				if(socketreterr(rt, sizeof(UINT8))){ break; }
				UINT8 subLen = *((UINT8 *)rt.data);
				__free(rt.data);
				offset += sizeof(UINT8);
				if(subLen == 0){ break; }
				offset += subLen;
			}

			UINT64 rdbTotalSize = offset - rdbStartOffset;
			rt = socketfcall(gif, read, rdbStartOffset, rdbTotalSize);
			gifRasterDataBlock_t *rdb = NULL;
			if(!socketreterr(rt, rdbTotalSize)){rdb = (gifRasterDataBlock_t *)rt.data;}

			//	Store Frame Record
			if(out->frames){
				out->frames = __realloc(out->frames, out->nFrames * sizeof(gifFrame_t), (out->nFrames + 1) * sizeof(gifFrame_t));
			}else{out->frames = __calloc(1, sizeof(gifFrame_t));}

			out->frames[out->nFrames].ld = ld;
			out->frames[out->nFrames].lcm = lcm;
			out->frames[out->nFrames].lcmLength = lcmLength;
			out->frames[out->nFrames].rdb = rdb;
			out->frames[out->nFrames].gce = activeGCE;
			out->nFrames++;

			//	Reset active GCE for future frames
			activeGCE = NULL;

			//	Set legacy frame-0 pointers
			if(out->nFrames == 1){
				out->ld = ld;
				out->lcm = lcm;
				out->lcmLength = lcmLength;
				out->rdb = rdb;
			}
		}else if(sentinel == 0x3B){//	Trailer (0x3B)
			offset += sizeof(UINT8);
			break;
		}else{break;}//	Unknown byte
	}

	return out;
}

UINT32 *GetGIFFrame(gifDescriptionSpace_t *desc, UINT32 FrameIndex, UINT32 *_Width, UINT32 *_Height, EFI_GRAPHICS_PIXEL_FORMAT PixelFormat){
    if(!desc || desc->nFrames == 0 || FrameIndex >= desc->nFrames || !_Width || !_Height){return NULL;}

    gifFrame_t *frame = &desc->frames[FrameIndex];
    if(!frame->rdb || !frame->ld){return NULL;}

    // Determine frame dimensions
    UINT32 width = frame->ld->ImageWidth, 
			height = frame->ld->ImageHeight;
    *_Width = width;
    *_Height = height;

    if(width == 0 || height == 0){return NULL;}

    UINT32 totalPixels = width * height;

    // Extract contiguous LZW byte stream from sub-blocks
    UINT8 *lzwStream = (UINT8 *)__calloc(1, totalPixels * 2);
    UINT32 lzwSize = 0;
    
    // Points to sub-blocks immediately following InitialCodeSize
    UINT8 *ptr = (UINT8 *)frame->rdb + sizeof(UINT8); 
    while(*ptr != 0){
        UINT8 blockLen = *ptr++;
        for(UINT8 i = 0; i < blockLen; i++){ lzwStream[lzwSize++] = *ptr++; }
    }

    // Initialize LZW Decoder state
    UINT8 initCodeSize = frame->rdb->InitialCodeSize;
    UINT32 clearCode = 1 << initCodeSize, 
           eoiCode = clearCode + 1, 
           codeSize = initCodeSize + 1, 
           maxCode = 1 << codeSize, 
           availableCode = eoiCode + 1;

    UINT16 prefix[4096] = {0};
    UINT8 suffix[4096] = {0}, 
          pixelStack[4097] = {0};
    UINT32 stackPtr = 0;
    for(UINT32 i = 0; i < clearCode; i++){suffix[i] = (UINT8)i;}
    UINT32 oldCode = 0xFFFF, 
           firstChar = 0, 
           bitPos = 0, 
           pixelIdx = 0;
    UINT8 *indices = (UINT8 *)__calloc(totalPixels, sizeof(UINT8));
    // Decode LZW stream to color-index array
    while(pixelIdx < totalPixels && (bitPos / 8) < lzwSize){
        UINT32 byteIdx = bitPos / 8, 
               bitOff = bitPos % 8, 
               code = ((UINT32)lzwStream[byteIdx] | ((UINT32)lzwStream[byteIdx + 1] << 8) | ((UINT32)lzwStream[byteIdx + 2] << 16)) >> bitOff;
        code &= (1 << codeSize) - 1;
        bitPos += codeSize;
        if(code == clearCode){
            codeSize = initCodeSize + 1;
            maxCode = 1 << codeSize;
            availableCode = eoiCode + 1;
            oldCode = 0xFFFF;
            continue;
        }
        if(code == eoiCode){break;}
        UINT32 inCode = code;
        if(code >= availableCode){
            pixelStack[stackPtr++] = (UINT8)firstChar;
            code = oldCode;
        }
        while(code >= clearCode){
            pixelStack[stackPtr++] = suffix[code];
            code = prefix[code];
        }
        firstChar = suffix[code];
        pixelStack[stackPtr++] = (UINT8)firstChar;
        if(availableCode < 4096){
            prefix[availableCode] = (UINT16)oldCode;
            suffix[availableCode] = (UINT8)firstChar;
            availableCode++;
            if((availableCode >= maxCode) && (codeSize < 12)){
                codeSize++;
                maxCode = 1 << codeSize;
            }
        }
        oldCode = inCode;
        while(stackPtr > 0){ if(pixelIdx < totalPixels){ indices[pixelIdx++] = pixelStack[--stackPtr]; }else{ stackPtr--; } }
    }
    __free(lzwStream);

    // Check for transparent color in frame's Graphic Control Extension
    INT32 transparentIdx = -1;
    if(frame->gce){
        gifExtBlockLocalDescriptorExtensionData *gceData = (gifExtBlockLocalDescriptorExtensionData *)frame->gce->Data;
        if(gceData->TransparentColorExists){transparentIdx = gceData->TransparentColor;}
    }

    // Map indices to 32-bit Frame Buffer according to target Pixel Format
    UINT32 *frameBuffer = (UINT32 *)__calloc(totalPixels, sizeof(UINT32));
    gifGlobalColor_t *palette = (frame->ld->LocalColorMapExists && frame->lcm) ? (gifGlobalColor_t *)frame->lcm : (gifGlobalColor_t *)desc->gcm;
    for(UINT32 i = 0; i < totalPixels; i++){
        UINT8 idx = indices[i];
        if(transparentIdx >= 0 && idx == (UINT8)transparentIdx){frameBuffer[i] = 0x00000000;}//	Transparent (Alpha = 0)
		else if(palette){
            UINT8 r = palette[idx].Red, 
                  g = palette[idx].Green, 
                  b = palette[idx].Blue;

            if(PixelFormat == PixelRedGreenBlueReserved8BitPerColor){
                frameBuffer[i] = (0xFF000000) | (b << 16) | (g << 8) | r;}//	Memory Order: [R, G, B, Reserved]
			else{frameBuffer[i] = (0xFF000000) | (r << 16) | (g << 8) | b;}//	Default / PixelBlueGreenRedReserved8BitPerColor: [B, G, R, Reserved]
        }
    }
    __free(indices);
    return frameBuffer;
}

/**
 * BltGIFFrame
 * Bit Block Transfers a decoded GIF frame onto a destination framebuffer.
 *
 * @param FrameBuffer    Pointer to decoded GIF frame pixel array (from GetGIFFrame).
 * @param FrameWidth     Width of the decoded frame in pixels.
 * @param FrameHeight    Height of the decoded frame in pixels.
 * @param DestBuffer     Pointer to target video framebuffer / surface.
 * @param DestX          X offset inside destination buffer to place the frame.
 * @param DestY          Y offset inside destination buffer to place the frame.
 * @param DestWidth      Total width of destination buffer (in pixels).
 * @param DestHeight     Total height of destination buffer (in pixels).
 * @param DestStride     Scanline stride of destination buffer (PixelsPerScanLine).
 */
void BltGIFFrame(const UINT32 *FrameBuffer, UINT32 FrameWidth, UINT32 FrameHeight, 
                 UINT32 *DestBuffer, INT32 DestX, INT32 DestY, 
                 UINT32 DestWidth, UINT32 DestHeight, UINT32 DestStride){
    if(!FrameBuffer || !DestBuffer || DestStride == 0){return;}

    // Calculate source/destination clipping bounds
    INT32 startY = (DestY < 0) ? -DestY : 0, 
		endY   = ((DestY + (INT32)FrameHeight) > (INT32)DestHeight) ? (INT32)DestHeight - DestY : (INT32)FrameHeight;

    INT32 startX = (DestX < 0) ? -DestX : 0, 
		endX   = ((DestX + (INT32)FrameWidth) > (INT32)DestWidth) ? (INT32)DestWidth - DestX : (INT32)FrameWidth;

    if(startX >= endX || startY >= endY){return;}
    for(INT32 y = startY; y < endY; ++y){
        const UINT32 *srcRow = FrameBuffer + (y * FrameWidth);
        UINT32 *dstRow = DestBuffer + ((DestY + y) * DestStride) + DestX;
        for(INT32 x = startX; x < endX; ++x){
            UINT32 pixel = srcRow[x];
            // Key out fully transparent pixels (Alpha = 0x00)
            if((pixel & 0xFF000000) != 0){dstRow[x] = pixel;}
        }
    }
}