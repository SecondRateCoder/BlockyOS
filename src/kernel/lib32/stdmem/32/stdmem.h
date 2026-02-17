#pragma once

#include "./kernel/lib32/generic/standard.h"
#include "./kernel/lib32/stdmemory/memory.h"

#define toID(key) ((uint32_t)(key)) ^ (((uint32_t)(key)) >> 16)
#define getbumpstate (bumpstate(0, 0, 0))


typedef enum E820BlockTypes{
    E820BlockType_Free = 0x01,
    E820BlockType_Reserved = 0x02,
    E820BlockType_ACPI = 0x03,
    E820BlockType_ACPINVS = 0x04
}E820BlockTypes;

typedef struct E820MemBlock{
    size_t  Base,
            Limit;
    uint32_t Type
}PACKEDSTRUCT E820MemBlock;
#define MemBlock E820MemBlock

typedef struct MemInfo{
    E820MemBlock *E820regions;
    uint32_t Count;
}PACKEDSTRUCT MemInfo;

typedef struct BitMapAllocatorState{
    uint8_t *Bitmap;
    size_t bitMapSize;
    uint32_t reserved;
    uint32_t reservedlength;
	uint32_t blockSize;
	MemInfo memory;
	uint32_t snapshot;
}PACKEDSTRUCT BitMapAllocatorState;

typedef struct DynamicMemoryMeta{
	uint32_t Hash;
	void *start;
	uint32_t size;
	void *next;
}DynamicMemoryMeta;

typedef struct SubBlockAllocatorState{
	uint32_t ID;
	uint32_t entries;
	DynamicMemoryMeta *memory;
}SubBlockAllocatorState;

static BitMapAllocatorState bitmapstate(uint8_t property, uint8_t state, uint32_t value);
