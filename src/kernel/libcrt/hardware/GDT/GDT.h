#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/memory/memory.h"
#include "TSS.h"

#define GDTTableDefaultLength  (UINT16_MAX / sizeof(GDTDescriptor))

typedef struct{
	uint16_t    Limit;
	uint64_t    Base;
}__packed GDTR64, LDTR64;

enumdef(uint8_t, GDTDescriptorAccessBits){
	GDAPresent = (1 << 7), GDAPriviledgeHigh = (3 << 5), 
	GDAPriviledgeMiddleHigh = (2 << 5), GDAPriviledgeMiddleLow = (1 << 5), 
	GDAPriviledgeLow = (0 << 5), GDANonSystemType = (1 << 4), 
	GDACodeDataType = (0 << 4), GDAExecutable = (1 << 3), 
	//	DC: Direction bit/Conforming bit.
    //		For data selectors: Direction bit. 
	//			If clear (0) the segment grows up. 
	//			If set (1) the segment grows down, ie. the Offset has to be greater than the Limit.
	//	For code selectors: Conforming bit. 
	//		If clear (0) code in this segment can only be executed from the ring set in DPL.
	//		If set (1) code in this segment can be executed from an equal or lower privilege level. 
	//			For example, code in ring 3 can far-jump to conforming code in a ring 2 segment. 
	//			The DPL field represent the highest privilege level that is allowed to execute the segment. 
	//			For example, code in ring 0 cannot far-jump to a conforming code segment where DPL is 2, while code in ring 2 and 3 can. 
	//			Note that the privilege level remains the same, ie. a far-jump from ring 3 to a segment with a DPL of 2 remains in ring 3 after the jump.
	GDADirectionDown = (1 << 2), GDADirectionUp = (0 << 2), 
	//	For code segments: Readable bit. 
	//		If clear (0), read access for this segment is not allowed. 
	//		If set (1) read access is allowed. Write access is never allowed for code segments.
	GDAReadable = (1 << 1), 
	//	For data segments: Writeable bit. 
	//		If clear (0), write access for this segment is not allowed. 
	//		If set (1) write access is allowed. Read access is always allowed for data segments.
	GDAWritable = (1 << 1), 
	GDAAccessed = (1)
};

typedef volatile struct{
	uint64_t	LimitLow				:	16;
	uint64_t	BaseLow					:	24;
	// uint64_t	AccessByte				:	8;
	uint64_t		ABAccessedBit		:	1;
	uint64_t		ABReadWritableBit	:	1;
	uint64_t		ABDirectionBit		:	1;
	uint64_t		ABExecutableBit		:	1;
	uint64_t		ABSystemSegmentBit	:	1;
	uint64_t		ABPriviledgeLevel	:	2;
	uint64_t		ABPresent			:	1;
	uint64_t	LimitHigh				:	4;
	// uint64_t	Flags					:	4;
	uint64_t		FGranularity		:	1;
	uint64_t		F32BitModeBit		:	1;
	uint64_t		FLongModeBit		:	1;
	uint64_t		FReserved			:	1;
	uint64_t	BaseHigh				:	8;
}__packed GDTDescriptor;

enumdef(uint8_t, GDSSFlags){
	Granularity1Byte = 0x0, Granularity4KB = 0x1, 
	_32BitSystemSegment = 0x4, _64BitSystemSegment = 0x8, 
	_LongModeSystemSegment = _64BitSystemSegment
};

//	Linear Addresses are the Inputs passed 

enumdef(uint8_t, GDSSType64){TypeLDT = 0x2, TSS64BitAvailable = 0x9, TSS64BitBusy = 0xB};
typedef volatile union{
	uint128_t		Raw;
	struct{
		uint64_t	LimitLow				:	16;
		uint64_t	LinearBaseLow			:	24;
		uint64_t		ABType				:	5;
		uint64_t		ABSystemSegmentBit	:	1;
		uint64_t		ABPriviledgeLevel	:	2;
		uint64_t		ABPresent			:	1;
		uint64_t	LimitHigh				:	4;
		uint64_t		FGranularity		:	1;
		uint64_t		F32BitModeBit		:	1;
		uint64_t		FLongModeBit		:	1;
		uint64_t							:	1;
		uint64_t	LinearBaseHigh			:	40;
		uint64_t							:	0;
	};
}__packed GDTSystemSegmentDescriptor64;

LibAPI bool LoadGDTR(GDTR64 *Register);
LibAPI bool ReadGDTR(GDTR64 *Register);
LibAPI bool AllocateSystemDescriptor(void *PhysicalBase, uint32_t Limit, 
	uint8_t Priviledge, GDSSType64 Type, GDSSFlags flags);