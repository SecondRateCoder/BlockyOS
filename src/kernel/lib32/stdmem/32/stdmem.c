#include "stdmem.h"

extern uint8_t __BITMAP_RESERVED;

bool InitBitmapAllocator(MemInfo in, void *reserved, uint32_t reservedsize, uint32_t blocksize){
	// Zero-out
	bitmapstate(1, 0, (ptrdigit)(&__BITMAP_RESERVED));
	bitmapstate(4, 0, (ptrdigit)reserved);
	bitmapstate(5, 0, reservedsize);
	bitmapstate(6, 0, blocksize);
	bitmapstate(7, 0, (ptrdigit)&in);
	bitmapstate(8, 0, 0);
	// Generate Bitmap Size
	size_t minsize = 0;
	for(uint32_t cc = 0; cc < in.Count; ++cc){minsize += in.E820regions[cc].Limit;}
	minsize /= blocksize;
	bitmapstate(2, 0, minsize);
	bitmapstate(3, 0, minsize >> 32);
	bitmapMark(0, minsize / blocksize, 0);
	bitmapMark(
		((ptrdigit)reserved / blocksize) + ((ptrdigit)reserved % blocksize != 0), 
		(reservedsize / blocksize) + (reservedsize % blocksize != 0),
		1
	);
	for(uint32_t cc = 0; cc < in.Count; ++cc){
		if(in.E820regions[cc].Type != E820BlockType_Free){
			bitmapMark(
				(in.E820regions[cc].Base / blocksize) + (in.E820regions[cc].Base % blocksize != 0), 
				(in.E820regions[cc].Limit / blocksize) + (in.E820regions[cc].Limit % blocksize != 0),
				1
			);
		}
	}
}

void bitmapMark(size_t index, ptrdigit pages, bool value){
	BitMapAllocatorState state = bitmapstate(0, 0, 0);
	// Needs to mark on a Bit state
	while((index % 8) != 0){
		if(value){state.Bitmap[index / 8] &= (value << (index % 8));}
		else{state.Bitmap[index / 8] &= ~(value << (index % 8));}
		index++;
	}
	memset(state.Bitmap + (index / 8), pages / 8, value);
	while((pages % 8) != 0){
		if(value){state.Bitmap[(pages + index) / 8] &= (value << (pages % 8));}
		else{state.Bitmap[(pages + index) / 8] &= ~(value << (pages % 8));}
		index++;
	}
}

void *flatpageAlloc(uint32_t size){
	BitMapAllocatorState state = bitmapstate(0, 0, 0);
	uint8_t retries = 200;
	while((state.Bitmap[(state.snapshot / 8)] >> (state.snapshot % 8)) & 0x1){
		if(state.snapshot < state.bitMapSize){state.snapshot++;}
		else{
			if(!retries){break;}
			state.snapshot = 0;
			retries--;
		}
	}
	bitmapMark(state.snapshot, ptrtoindex(state, size), 1);
	uint32_t cc = 0;
	E820MemBlock *memblock = state.memory.E820regions;
	for(uint32_t cc = 0; cc < state.memory.Count; ++cc, memblock = state.memory.E820regions + cc){
		if(iswithin(state.snapshot, size, memblock->Base, memblock->Limit)){
			memcpy((void *)(memblock->Base + (state.snapshot * state.blockSize)), &size, sizeof(size_t));
			return (void *)(memblock->Base + (state.snapshot * state.blockSize) + sizeof(size_t));
		}
	}
}

void flatPageFree(void *memory){
	BitMapAllocatorState state = bitmapstate(0, 0, 0);
	bitmapMark(ptrtoindex(state, memory), (ptrdigit)(*(ptrdigit *)(memory - sizeof(size_t))), 0);
	return;
}

void treeFree(void *memory){
	DynamicMemoryMeta *block = memory - sizeof(DynamicMemoryMeta);
	if(block->last){
		((DynamicMemoryMeta *)block->last)->next = block->next;
		((DynamicMemoryMeta *)block->last)->next = NULL;
	}
	return;
}

void *treeAlloc(uint32_t bytes, void *lastAlloc){
	BitMapAllocatorState state = bitmapstate(0, 0, 0);
	DynamicMemoryMeta *base = (DynamicMemoryMeta *)(lastAlloc - ((ptrdigit)lastAlloc % state.blockSize) + sizeof(size_t));
	DynamicMemoryMeta *current = base;
	while(current->next != NULL){current = current->next;}
	current->next = current->next + current->next->size;
	void *out = current->next;
	//! Build Block
	return out;
}

/// @brief Get/Set the BitmapAllocatorState
/// @param property The properties of the state; indexed as 32-bit blocks. Must be non-zero
/// @param state The type of artithmetic to be done, 0: Assign, 1: Add, 2: Subtract. Although some can only be writte/read to.
/// @param value The value to Assign/Add/Subtract.
/// @return The BitmapState; post-assignment.
static BitMapAllocatorState bitmapstate(uint8_t property, uint8_t state, ptrdigit value){
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
				case 0: {allocstate.bitMapSize &= value;}
				case 1: {allocstate.bitMapSize |= allocstate.bitMapSize + value;}
				case 2: {allocstate.bitMapSize |= allocstate.bitMapSize - value;}
			}
		}
		case 3: {
			switch(state){
				case 0: {allocstate.bitMapSize &= (((size_t)value) << 32);}
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
