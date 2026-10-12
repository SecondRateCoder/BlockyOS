#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/memory/memory.h"
#include "TSS.h"

#define GDTTableDefaultLength  ((UINT16_MAX + 1) / sizeof(GDTDescriptor))

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

typedef volatile struct {
    // --- Lower 32 Bits ---
    uint64_t LimitLow            : 16; // Bits 0..15  : Segment Limit [15:0]
    uint64_t BaseLow             : 24; // Bits 16..39 : Base Address [23:0]
    
    // --- Access Byte (Bits 40..47) ---
    uint64_t ABAccessedBit       : 1;  // Bit 40      : Accessed bit
    uint64_t ABReadWritableBit   : 1;  // Bit 41      : Readable/Writable
    uint64_t ABDirectionBit      : 1;  // Bit 42      : Conforming/Direction
    uint64_t ABExecutableBit     : 1;  // Bit 43      : Executable (1=Code, 0=Data)
    uint64_t ABSystemSegmentBit  : 1;  // Bit 44      : Descriptor Type (1=Code/Data, 0=Sys)
    uint64_t ABPriviledgeLevel   : 2;  // Bits 45..46 : DPL (0 = Ring 0)
    uint64_t ABPresent           : 1;  // Bit 47      : Present bit
    
    // --- Upper 16 Bits ---
    uint64_t LimitHigh           : 4;  // Bits 48..51 : Segment Limit [19:16]
    
    // --- Flags Nibble (Bits 52..55) FIXED ORDER ---
    uint64_t FAvailable          : 1;  // Bit 52      : Available for OS software (AVL)
    uint64_t FLongModeBit        : 1;  // Bit 53      : 64-bit Long Mode Code (L)
    uint64_t F32BitModeBit       : 1;  // Bit 54      : Default / 32-bit Segment (D/B)
    uint64_t FGranularity        : 1;  // Bit 55      : Granularity (0=Byte, 1=4KB)
    
    uint64_t BaseHigh            : 8;  // Bits 56..63 : Base Address [31:24]
} __packed GDTDescriptor;

enumdef(uint8_t, GDSSFlags){
	Granularity1Byte = 0x0, Granularity4KB = 0x1, 
	_32BitSystemSegment = 0x4, _64BitSystemSegment = 0x8, 
	_LongModeSystemSegment = _64BitSystemSegment
};

//	Linear Addresses are the Inputs passed 

enumdef(uint8_t, GDSSType64){TypeLDT = 0x2, TSS64BitAvailable = 0x9, TSS64BitBusy = 0xB};
typedef volatile union{
    uint128_t Raw;
    struct{
        // --- Lower 64 Bits (Descriptor Low) ---
        uint64_t LimitLow			: 16; // Bits 0..15  : Limit [15:0]
        uint64_t BaseLow			: 24; // Bits 16..39 : Base [23:0]
        
        // Access Byte (Bits 40..47)
        uint64_t ABType				: 4;  // Bits 40..43 : System Type (FIXED: 4 bits, e.g. 0x9=TSS)
        uint64_t ABSystemSegmentBit	: 1;  // Bit 44      : S = 0 for System Segment
        uint64_t ABPriviledgeLevel	: 2;  // Bits 45..46 : DPL
        uint64_t ABPresent			: 1;  // Bit 47      : Present bit
        
        uint64_t LimitHigh			: 4;  // Bits 48..51 : Limit [19:16]
        
        // Flags Nibble (Bits 52..55)
        uint64_t FAvailable			: 1;  // Bit 52      : AVL
        uint64_t 					: 2;  // Bits 53..54 : Reserved (Must be 0 for TSS)
        uint64_t FGranularity		: 1;  // Bit 55      : Granularity (0=Byte, 1=4KB)

        // --- Upper 64 Bits (Descriptor High) ---
        uint64_t BaseHigh			: 40; // Bits 64..95 : Base [63:24]
        uint64_t 					: 32; // Bits 96..127: Reserved (Must be 0)
    };
} __packed GDTSystemSegmentDescriptor64;

LibAPI bool LoadGDTR(GDTR64 *Register);
LibAPI bool ReadGDTR(GDTR64 *Register);
extern LibAPI bool __sysvabi _LoadGDTR(GDTR64 *Register);
extern LibAPI bool __sysvabi _ReadGDTR(GDTR64 *Register);
extern LibAPI bool __sysvabi _LoadLDTR(GDTR64 *Register);
extern LibAPI bool __sysvabi _ReadLDTR(GDTR64 *Register);
LibAPI bool AllocateSystemDescriptor(void *PhysicalBase, uint32_t Limit, 
	uint8_t Priviledge, GDSSType64 Type, GDSSFlags flags);