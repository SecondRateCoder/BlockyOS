#include "stdmem.h"

bool InitBitmapAllocator(MemInfo in, void *bitmap, void *reserved, uint32_t reservedsize, uint32_t blocksize){
	// Zero-out
	bitmapstate(1, 0, (uint32_t)bitmap);
	bitmapstate(4, 0, (uint32_t)reserved);
	bitmapstate(5, 0, reservedsize);
	bitmapstate(6, 0, blocksize);
	bitmapstate(7, 0, (uint32_t)&in);
	bitmapstate(8, 0, 0);
	// Generate Bitmap Size
	size_t minsize = 0;
	for(uint32_t cc = 0; cc < in.Count; ++cc){if(in.E820regions[cc].Type == E820BlockType_Free){minsize += in.E820regions[cc].Limit;}}
	minsize /= blocksize;
	bitmapstate(2, 0, minsize);
	bitmapstate(3, 0, minsize >> 32);
	// Mark used and unused
	for(uint32_t cc = 0; cc < in.Count; ++cc){
		if(in.E820regions[cc].Type == E820BlockType_Free){
			memset(bitmap + (in.E820regions[cc].Base / blocksize), in.E820regions[cc].Limit / blocksize, 0);
		}else{memset(bitmap + (in.E820regions[cc].Base / blocksize), in.E820regions[cc].Limit / blocksize, 0xFF);}
		if((reserved > in.E820regions[cc].Base) && (reserved < (in.E820regions[cc].Base + in.E820regions[cc].Limit)))
		{memset(bitmap + ((in.E820regions[cc].Base + (uint32_t)reserved) / blocksize), min(reservedsize, in.E820regions[cc].Limit), 0xFF);}
	}
}

/// @brief Get/Set the BitmapAllocatorState
/// @param property The properties of the state; indexed as 32-bit blocks. Must be non-zero
/// @param state The type of artithmetic to be done, 0: Assign, 1: Add, 2: Subtract. Although some can only be writte/read to.
/// @param value The value to Assign/Add/Subtract.
/// @return The BitmapState; post-assignment.
static BitMapAllocatorState bitmapstate(uint8_t property, uint8_t state, uint32_t value){
	static BitMapAllocatorState allocstate;
	switch(property){
		case 1: {
			switch(state){
				case 0: {allocstate.Bitmap = (void *)value;}
				case 1: {allocstate.Bitmap += value;}
				case 2: {allocstate.Bitmap -= value;}
			}
		}
		case 2: {
			switch(state){
				case 0: {allocstate.bitMapSize |= value;}
				case 1: {allocstate.bitMapSize |= allocstate.bitMapSize + value;}
				case 2: {allocstate.bitMapSize |= allocstate.bitMapSize - value;}
			}
		}
		case 3: {
			switch(state){
				case 0: {allocstate.bitMapSize |= (((size_t)value) << 32);}
				case 1: {allocstate.bitMapSize |= allocstate.bitMapSize + (((size_t)value) << 32);}
				case 2: {allocstate.bitMapSize |= allocstate.bitMapSize - (((size_t)value) << 32);}
			}
		}
		case 4: {
			switch(state){
				case 0: {allocstate.reserved = value;}
				case 1: {allocstate.reserved += value;}
				case 2: {allocstate.reserved -= value;}
			}
		}
		case 5: {
			switch(state){
				case 0: {allocstate.reservedlength = value;}
				case 1: {allocstate.reservedlength += value;}
				case 2: {allocstate.reservedlength -= value;}
			}
		}
		case 6: {
			switch(state){
				case 0: {allocstate.blockSize = value;}
				case 1: {allocstate.blockSize += value;}
				case 2: {allocstate.blockSize -= value;}
			}
		}
		case 7: {
			switch(state){
				case 0: {allocstate.memory = *((MemInfo *)((void *)value));}
			}
		}
		case 8: {
			switch(state){
				case 0: {allocstate.snapshot = value;}
				case 1: {allocstate.snapshot += value;}
				case 2: {allocstate.snapshot -= value;}
			}
		}
	}
	return allocstate;
}